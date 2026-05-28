#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include <array>
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>

#include "Arduino.h"
#include "IRremote.hpp"

#include "../FP317_driver.cpp"
#include "../FP317_gfx.cpp"

void resetGame();
void placeApple();
void drawInitialGame();
void drawCell(int x, int y, bool on);
void drawApple();
void spiralFill();
void gameOver();

#include "../FP317-Driver.ino"

namespace {
constexpr uint8_t IR_UP_SIM = 0x58;
constexpr uint8_t IR_DOWN_SIM = 0x59;
constexpr uint8_t IR_LEFT_SIM = 0x5A;
constexpr uint8_t IR_RIGHT_SIM = 0x5B;
constexpr uint8_t IR_ENTER_SIM = 0x5C;

constexpr int kCellSize = 22;
constexpr int kMargin = 18;
constexpr int kDotRadiusX = 8;
constexpr int kDotRadiusY = 9;
constexpr int kWindowWidth = GAME_COLS * kCellSize + kMargin * 2;
constexpr int kWindowHeight = GAME_ROWS * kCellSize + kMargin * 2;

std::array<std::array<bool, GAME_COLS>, GAME_ROWS> framebuffer{};
std::array<std::array<bool, GAME_COLS>, GAME_ROWS> dirtyDots{};
std::mutex framebufferMutex;
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
bool needsRender = true;
bool needsFullRender = true;
std::atomic<bool> quitRequested{false};
std::atomic<bool> gameThreadDone{false};

bool pinHigh(int pin) {
  auto it = SimArduino::pinStates.find(pin);
  return it != SimArduino::pinStates.end() && it->second == HIGH;
}

int encodedBits(int a0, int a1, int a2) {
  return (pinHigh(a0) ? 1 : 0) | (pinHigh(a1) ? 2 : 0) |
         (pinHigh(a2) ? 4 : 0);
}

int encodedBits(int b0, int b1) {
  return (pinHigh(b0) ? 1 : 0) | (pinHigh(b1) ? 2 : 0);
}

void latchDisplayFromPins(int enablePin) {
  for (int d = 0; d < sizeof(displays) / sizeof(displays[0]); d++) {
    const FP317_module& display = displays[d];
    if (!display.enabled || display.PIN_ENABLE != enablePin) {
      continue;
    }

    const bool evenRow = display.gridY % 2 == 0;
    const int encodedXGroup = encodedBits(PIN_U1_B0, PIN_U1_B1);
    const int encodedXPixel = encodedBits(PIN_U1_A0, PIN_U1_A1, PIN_U1_A2);
    const int encodedYGroup = pinHigh(PIN_U2_B1) ? 1 : 0;
    const int encodedYPixel = encodedBits(PIN_U2_A0, PIN_U2_A1, PIN_U2_A2);

    const int xGroup = evenRow ? encodedXGroup : 3 - encodedXGroup;
    const int xPixel = evenRow ? encodedXPixel : 8 - encodedXPixel;
    const int yGroup = evenRow ? 1 - encodedYGroup : encodedYGroup;
    const int yPixel = evenRow ? encodedYPixel : 8 - encodedYPixel;

    const int x = (display.gridX - 1) * 28 + xGroup * 7 + xPixel - 1;
    const int y = (display.gridY - 1) * 14 + yGroup * 7 + yPixel - 1;
    const bool on = evenRow ? !pinHigh(PIN_U1_DATA) : pinHigh(PIN_U1_DATA);

    if (x >= 0 && x < GAME_COLS && y >= 0 && y < GAME_ROWS) {
      std::lock_guard<std::mutex> lock(framebufferMutex);
      if (framebuffer[y][x] != on || needsFullRender) {
        framebuffer[y][x] = on;
        dirtyDots[y][x] = true;
      }
      needsRender = true;
    }
    return;
  }
}

void installDisplayHook() {
  SimArduino::digitalWriteHook = [](int pin, int value) {
    if (value == HIGH) {
      latchDisplayFromPins(pin);
    }
  };
}

void drawFilledEllipse(int cx, int cy, int rx, int ry) {
  const int rx2 = rx * rx;
  const int ry2 = ry * ry;
  const int limit = rx2 * ry2;
  for (int y = -ry; y <= ry; y++) {
    for (int x = -rx; x <= rx; x++) {
      if (x * x * ry2 + y * y * rx2 <= limit) {
        SDL_RenderDrawPoint(renderer, cx + x, cy + y);
      }
    }
  }
}

void drawFlipDot(int x, int y, bool on) {
  const int cx = kMargin + x * kCellSize + kCellSize / 2;
  const int cy = kMargin + y * kCellSize + kCellSize / 2;

  SDL_SetRenderDrawColor(renderer, 4, 5, 6, 255);
  drawFilledEllipse(cx + 1, cy + 1, kDotRadiusX + 3, kDotRadiusY + 3);

  if (on) {
    SDL_SetRenderDrawColor(renderer, 226, 205, 89, 255);
    drawFilledEllipse(cx, cy, kDotRadiusX + 1, kDotRadiusY + 1);
    SDL_SetRenderDrawColor(renderer, 248, 234, 137, 255);
    drawFilledEllipse(cx - 2, cy - 2, kDotRadiusX - 3, kDotRadiusY - 4);
    SDL_SetRenderDrawColor(renderer, 119, 95, 39, 255);
    SDL_RenderDrawLine(renderer, cx - kDotRadiusX + 2, cy + kDotRadiusY - 2,
                       cx + kDotRadiusX - 1, cy - kDotRadiusY + 2);
  } else {
    SDL_SetRenderDrawColor(renderer, 17, 18, 18, 255);
    drawFilledEllipse(cx, cy, kDotRadiusX + 1, kDotRadiusY + 1);
    SDL_SetRenderDrawColor(renderer, 42, 43, 42, 255);
    drawFilledEllipse(cx - 2, cy - 2, kDotRadiusX - 4, kDotRadiusY - 5);
    SDL_SetRenderDrawColor(renderer, 6, 7, 8, 255);
    SDL_RenderDrawLine(renderer, cx - kDotRadiusX + 2, cy + kDotRadiusY - 2,
                       cx + kDotRadiusX - 1, cy - kDotRadiusY + 2);
  }
}

void drawPanel() {
  SDL_SetRenderDrawColor(renderer, 11, 12, 13, 255);
  SDL_RenderClear(renderer);

  SDL_Rect panel{kMargin - 8, kMargin - 8, GAME_COLS * kCellSize + 16,
                 GAME_ROWS * kCellSize + 16};
  SDL_SetRenderDrawColor(renderer, 26, 28, 29, 255);
  SDL_RenderFillRect(renderer, &panel);

  SDL_SetRenderDrawColor(renderer, 12, 13, 14, 255);
  for (int x = 7; x < GAME_COLS; x += 7) {
    const int px = kMargin + x * kCellSize;
    SDL_RenderDrawLine(renderer, px, kMargin - 7, px,
                       kMargin + GAME_ROWS * kCellSize + 7);
  }
  SDL_RenderDrawLine(renderer, kMargin - 7, kMargin + 7 * kCellSize,
                     kMargin + GAME_COLS * kCellSize + 7,
                     kMargin + 7 * kCellSize);
}

void render(bool force = false) {
  std::array<std::array<bool, GAME_COLS>, GAME_ROWS> frameSnapshot{};
  std::array<std::array<bool, GAME_COLS>, GAME_ROWS> dirtySnapshot{};
  bool fullRender = false;

  {
    std::lock_guard<std::mutex> lock(framebufferMutex);
    if (!force && !needsRender && !needsFullRender) {
      return;
    }

    frameSnapshot = framebuffer;
    dirtySnapshot = dirtyDots;
    fullRender = force || needsFullRender;

    needsRender = false;
    needsFullRender = false;
    for (auto& row : dirtyDots) {
      row.fill(false);
    }
  }

  if (fullRender) {
    drawPanel();
  }

  for (int y = 0; y < GAME_ROWS; y++) {
    for (int x = 0; x < GAME_COLS; x++) {
      if (fullRender || dirtySnapshot[y][x]) {
        drawFlipDot(x, y, frameSnapshot[y][x]);
      }
    }
  }

  SDL_RenderPresent(renderer);
}

void injectDirection(SDL_Keycode key) {
  if (key == SDLK_UP || key == SDLK_w) IrReceiver.inject(IR_UP_SIM);
  if (key == SDLK_DOWN || key == SDLK_s) IrReceiver.inject(IR_DOWN_SIM);
  if (key == SDLK_LEFT || key == SDLK_a) IrReceiver.inject(IR_LEFT_SIM);
  if (key == SDLK_RIGHT || key == SDLK_d) IrReceiver.inject(IR_RIGHT_SIM);
}

void serviceSdl() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      quitRequested = true;
    } else if (event.type == SDL_KEYDOWN && !event.key.repeat) {
      SDL_Keycode key = event.key.keysym.sym;
      injectDirection(key);
      if (key == SDLK_RETURN || key == SDLK_r) IrReceiver.inject(IR_ENTER_SIM);
      if (key == SDLK_ESCAPE || key == SDLK_q) quitRequested = true;
    }
  }
}

void initSdl() {
  SDL_SetMainReady();
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    throw std::runtime_error(SDL_GetError());
  }

  window = SDL_CreateWindow("FP317 Simulator", SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, kWindowWidth, kWindowHeight,
                            SDL_WINDOW_SHOWN);
  if (!window) {
    throw std::runtime_error(SDL_GetError());
  }

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (!renderer) {
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
  }
  if (!renderer) {
    throw std::runtime_error(SDL_GetError());
  }
}

void shutdownSdl() {
  if (renderer) SDL_DestroyRenderer(renderer);
  if (window) SDL_DestroyWindow(window);
  SDL_Quit();
}

void runGame() {
  try {
    setup();
    while (!quitRequested) {
      loop();
    }
  } catch (const std::exception& ex) {
    std::cerr << "Game thread error: " << ex.what() << '\n';
    quitRequested = true;
  }
  gameThreadDone = true;
}
}  // namespace

int main(int argc, char** argv) {
  int tickLimit = 0;
  if (argc == 3 && std::string(argv[1]) == "--ticks") {
    tickLimit = std::atoi(argv[2]);
  }

  try {
    std::srand(1);
    initSdl();
    SimArduino::stopDelayHook = []() { return quitRequested.load(); };
    installDisplayHook();
    render(true);

    std::thread gameThread(runGame);

    int ticks = 0;
    while (!quitRequested && !gameThreadDone &&
           (tickLimit == 0 || ticks++ < tickLimit)) {
      serviceSdl();
      render();
      SDL_Delay(16);
    }
    quitRequested = true;

    if (gameThread.joinable()) {
      gameThread.join();
    }

    shutdownSdl();
  } catch (const std::exception& ex) {
    std::cerr << "Simulator error: " << ex.what() << '\n';
    shutdownSdl();
    return 1;
  }

  return 0;
}
