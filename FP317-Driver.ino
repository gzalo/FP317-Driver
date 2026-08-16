/*
 * File: FP317.ino
 * Implements a tiny game menu for a 28 x 14 FP317 flip-dot display.
 *
 * Created by Andrew (Novar Lynx) (C) 2022
 */

#include "FP317_gfx.h"
#include "Ferranti7.h"
#include <Fonts/TomThumb.h>
//#include <Fonts/FreeSans9pt7b.h>
#include <IRremote.hpp>

FP317_gfx* gfx;

#define GAME_COLS 28
#define GAME_ROWS 14
#define IR_RECEIVE_PIN 53

// IR codes
#define IR_UP    0x58
#define IR_DOWN  0x59
#define IR_LEFT  0x5A
#define IR_RIGHT 0x5B
#define IR_ENTER 0x5C
#define IR_DISC  0x3F

#define CELL_ON 1
#define CELL_OFF 0

enum AppMode {
  MODE_MENU,
  MODE_PLAYING,
  MODE_GAME_OVER
};

enum GameId {
  GAME_SNAKE,
  GAME_TETRIS,
  GAME_FROGGER,
  GAME_BOMBERMAN,
  GAME_BREAKOUT,
  GAME_INVADERS,
  GAME_COUNT
};

AppMode appMode = MODE_MENU;
GameId selectedGame = GAME_SNAKE;
GameId activeGame = GAME_SNAKE;
int lastScore = 0;
bool lastWin = false;
unsigned long gameOverShownAt = 0;
unsigned long menuInputLockedUntil = 0;
bool screenCells[GAME_ROWS][GAME_COLS];
bool frameCells[GAME_ROWS][GAME_COLS];
bool screenReady = false;
bool drawingFrame = false;

void drawCell(int x, int y, bool on);
void clearScreen();
void presentScreen();
void drawCenteredText(const char* text, int y);
void drawCenteredNumber(int value, int y);
int textWidth(const char* text);
void drawMenu();
void handleMenuInput(uint8_t cmd);
void startSelectedGame();
void showGameOver(int score, bool win);
void handleGameOverInput(uint8_t cmd);
void drawIcon(GameId game, int x, int y);

void resetSnake();
void handleSnakeInput(uint8_t cmd);
void updateSnake();
void drawInitialSnake();
void placeApple();
void drawApple();
void spiralFill();

void resetTetris();
void handleTetrisInput(uint8_t cmd);
void updateTetris();
void drawTetris();

void resetFrogger();
void handleFroggerInput(uint8_t cmd);
void updateFrogger();
void drawFrogger();

void resetBomberman();
void handleBombermanInput(uint8_t cmd);
void updateBomberman();
void drawBomberman();
struct Bomb;
bool inBombBlastLine(int x, int y, Bomb* bomb);
void drawBlastForBomb(Bomb* bomb);

void resetBreakout();
void handleBreakoutInput(uint8_t cmd);
void updateBreakout();
void drawBreakout();

void resetInvaders();
void handleInvadersInput(uint8_t cmd);
void updateInvaders();
void drawInvaders();

void setup() {
  Serial.begin(115200);
  gfx = new FP317_gfx();
  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);
  gfx->setLag(0);
  clearScreen();
  delay(700);
  drawMenu();
}

void loop() {
  if (IrReceiver.decode()) {
    uint8_t cmd = IrReceiver.decodedIRData.command;
    Serial.print("IR code: 0x");
    Serial.println(cmd, HEX);

    if (appMode == MODE_MENU) {
      handleMenuInput(cmd);
    } else if (appMode == MODE_GAME_OVER) {
      handleGameOverInput(cmd);
    } else if (cmd == IR_DISC) {
      appMode = MODE_MENU;
      drawMenu();
    } else if (activeGame == GAME_SNAKE) {
      handleSnakeInput(cmd);
    } else if (activeGame == GAME_TETRIS) {
      handleTetrisInput(cmd);
    } else if (activeGame == GAME_FROGGER) {
      handleFroggerInput(cmd);
    } else if (activeGame == GAME_BOMBERMAN) {
      handleBombermanInput(cmd);
    } else if (activeGame == GAME_BREAKOUT) {
      handleBreakoutInput(cmd);
    } else if (activeGame == GAME_INVADERS) {
      handleInvadersInput(cmd);
    }

    IrReceiver.resume();
  }

  if (appMode != MODE_PLAYING) {
    return;
  }

  if (activeGame == GAME_SNAKE) {
    updateSnake();
  } else if (activeGame == GAME_TETRIS) {
    updateTetris();
  } else if (activeGame == GAME_FROGGER) {
    updateFrogger();
  } else if (activeGame == GAME_BOMBERMAN) {
    updateBomberman();
  } else if (activeGame == GAME_BREAKOUT) {
    updateBreakout();
  } else if (activeGame == GAME_INVADERS) {
    updateInvaders();
  }
}

void clearScreen() {
  if (!screenReady) {
    gfx->clearDisplay();
    screenReady = true;
  }

  for (int y = 0; y < GAME_ROWS; y++) {
    for (int x = 0; x < GAME_COLS; x++) {
      frameCells[y][x] = false;
    }
  }
  drawingFrame = true;
}

void presentScreen() {
  if (!drawingFrame) return;

  for (int y = 0; y < GAME_ROWS; y++) {
    for (int x = 0; x < GAME_COLS; x++) {
      if (screenCells[y][x] != frameCells[y][x]) {
        gfx->fillRect(x, y, 1, 1, frameCells[y][x] ? CELL_ON : CELL_OFF);
        screenCells[y][x] = frameCells[y][x];
      }
    }
  }
  drawingFrame = false;
}

void drawCell(int x, int y, bool on) {
  if (x < 0 || x >= GAME_COLS || y < 0 || y >= GAME_ROWS) return;
  if (drawingFrame) {
    frameCells[y][x] = on;
    return;
  }
  if (screenReady && screenCells[y][x] == on) return;

  gfx->fillRect(x, y, 1, 1, on ? CELL_ON : CELL_OFF);
  screenCells[y][x] = on;
}

int textWidth(const char* text) {
  int width = 0;
  while (*text++) {
    width += 4;
  }
  return width;
}

bool tinyGlyph(char c, uint8_t rows[5]) {
  for (int i = 0; i < 5; i++) rows[i] = 0;
  if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';

  switch (c) {
    case 'A': rows[0] = 0b010; rows[1] = 0b101; rows[2] = 0b111; rows[3] = 0b101; rows[4] = 0b101; return true;
    case 'B': rows[0] = 0b110; rows[1] = 0b101; rows[2] = 0b110; rows[3] = 0b101; rows[4] = 0b110; return true;
    case 'C': rows[0] = 0b111; rows[1] = 0b100; rows[2] = 0b100; rows[3] = 0b100; rows[4] = 0b111; return true;
    case 'E': rows[0] = 0b111; rows[1] = 0b100; rows[2] = 0b110; rows[3] = 0b100; rows[4] = 0b111; return true;
    case 'F': rows[0] = 0b111; rows[1] = 0b100; rows[2] = 0b110; rows[3] = 0b100; rows[4] = 0b100; return true;
    case 'G': rows[0] = 0b111; rows[1] = 0b100; rows[2] = 0b101; rows[3] = 0b101; rows[4] = 0b111; return true;
    case 'I': rows[0] = 0b111; rows[1] = 0b010; rows[2] = 0b010; rows[3] = 0b010; rows[4] = 0b111; return true;
    case 'J': rows[0] = 0b001; rows[1] = 0b001; rows[2] = 0b001; rows[3] = 0b101; rows[4] = 0b010; return true;
    case 'K': rows[0] = 0b101; rows[1] = 0b101; rows[2] = 0b110; rows[3] = 0b101; rows[4] = 0b101; return true;
    case 'L': rows[0] = 0b100; rows[1] = 0b100; rows[2] = 0b100; rows[3] = 0b100; rows[4] = 0b111; return true;
    case 'M': rows[0] = 0b101; rows[1] = 0b111; rows[2] = 0b111; rows[3] = 0b101; rows[4] = 0b101; return true;
    case 'N': rows[0] = 0b101; rows[1] = 0b111; rows[2] = 0b111; rows[3] = 0b111; rows[4] = 0b101; return true;
    case 'O': rows[0] = 0b111; rows[1] = 0b101; rows[2] = 0b101; rows[3] = 0b101; rows[4] = 0b111; return true;
    case 'R': rows[0] = 0b110; rows[1] = 0b101; rows[2] = 0b110; rows[3] = 0b101; rows[4] = 0b101; return true;
    case 'S': rows[0] = 0b111; rows[1] = 0b100; rows[2] = 0b111; rows[3] = 0b001; rows[4] = 0b111; return true;
    case 'T': rows[0] = 0b111; rows[1] = 0b010; rows[2] = 0b010; rows[3] = 0b010; rows[4] = 0b010; return true;
    case 'U': rows[0] = 0b101; rows[1] = 0b101; rows[2] = 0b101; rows[3] = 0b101; rows[4] = 0b111; return true;
    case 'W': rows[0] = 0b101; rows[1] = 0b101; rows[2] = 0b111; rows[3] = 0b111; rows[4] = 0b101; return true;
    case 'Y': rows[0] = 0b101; rows[1] = 0b101; rows[2] = 0b010; rows[3] = 0b010; rows[4] = 0b010; return true;
    case ' ': return true;
    case '0': rows[0] = 0b111; rows[1] = 0b101; rows[2] = 0b101; rows[3] = 0b101; rows[4] = 0b111; return true;
    case '1': rows[0] = 0b010; rows[1] = 0b110; rows[2] = 0b010; rows[3] = 0b010; rows[4] = 0b111; return true;
    case '2': rows[0] = 0b111; rows[1] = 0b001; rows[2] = 0b111; rows[3] = 0b100; rows[4] = 0b111; return true;
    case '3': rows[0] = 0b111; rows[1] = 0b001; rows[2] = 0b111; rows[3] = 0b001; rows[4] = 0b111; return true;
    case '4': rows[0] = 0b101; rows[1] = 0b101; rows[2] = 0b111; rows[3] = 0b001; rows[4] = 0b001; return true;
    case '5': rows[0] = 0b111; rows[1] = 0b100; rows[2] = 0b111; rows[3] = 0b001; rows[4] = 0b111; return true;
    case '6': rows[0] = 0b111; rows[1] = 0b100; rows[2] = 0b111; rows[3] = 0b101; rows[4] = 0b111; return true;
    case '7': rows[0] = 0b111; rows[1] = 0b001; rows[2] = 0b010; rows[3] = 0b010; rows[4] = 0b010; return true;
    case '8': rows[0] = 0b111; rows[1] = 0b101; rows[2] = 0b111; rows[3] = 0b101; rows[4] = 0b111; return true;
    case '9': rows[0] = 0b111; rows[1] = 0b101; rows[2] = 0b111; rows[3] = 0b001; rows[4] = 0b111; return true;
  }
  return false;
}

void drawTinyText(const char* text, int x, int y) {
  while (*text) {
    uint8_t rows[5];
    tinyGlyph(*text++, rows);
    for (int row = 0; row < 5; row++) {
      for (int col = 0; col < 3; col++) {
        if (rows[row] & (1 << (2 - col))) {
          drawCell(x + col, y + row, true);
        }
      }
    }
    x += 4;
  }
}

void drawCenteredText(const char* text, int y) {
  int x = (GAME_COLS - textWidth(text)) / 2;
  if (x < 0) x = 0;
  drawTinyText(text, x, y - 5);
}

void drawCenteredNumber(int value, int y) {
  char text[8];
  int pos = 0;

  if (value < 0) value = 0;
  if (value == 0) {
    text[pos++] = '0';
  } else {
    char reversed[8];
    int count = 0;
    while (value > 0 && count < 7) {
      reversed[count++] = '0' + (value % 10);
      value /= 10;
    }
    while (count > 0) {
      text[pos++] = reversed[--count];
    }
  }

  text[pos] = '\0';
  drawCenteredText(text, y);
}

const char* gameName(GameId game) {
  if (game == GAME_SNAKE) return "SNAKE";
  if (game == GAME_TETRIS) return "TETRIS";
  if (game == GAME_FROGGER) return "FROG";
  if (game == GAME_BOMBERMAN) return "BOMB";
  if (game == GAME_BREAKOUT) return "BRK";
  return "ALIEN";
}

void drawMenu() {
  clearScreen();
  drawCenteredText(gameName(selectedGame), 6);

  const int menuIconX[GAME_COUNT] = {0, 5, 10, 14, 19, 24};
  for (int i = 0; i < GAME_COUNT; i++) {
    int x = menuIconX[i];
    drawIcon((GameId)i, x, 8);
    if (i == selectedGame) {
      drawCell(x + 1, 13, true);
      drawCell(x + 2, 13, true);
    }
  }
  presentScreen();
}

void drawIcon(GameId game, int x, int y) {
  if (game == GAME_SNAKE) {
    drawCell(x + 0, y + 2, true);
    drawCell(x + 1, y + 2, true);
    drawCell(x + 2, y + 2, true);
    drawCell(x + 3, y + 1, true);
    drawCell(x + 3, y + 2, true);
  } else if (game == GAME_TETRIS) {
    drawCell(x + 1, y + 0, true);
    drawCell(x + 1, y + 1, true);
    drawCell(x + 1, y + 2, true);
    drawCell(x + 2, y + 2, true);
    drawCell(x + 3, y + 2, true);
  } else if (game == GAME_FROGGER) {
    drawCell(x + 0, y + 1, true);
    drawCell(x + 2, y + 1, true);
    drawCell(x + 1, y + 2, true);
    drawCell(x + 3, y + 2, true);
  } else if (game == GAME_BOMBERMAN) {
    drawCell(x + 2, y + 0, true);
    drawCell(x + 1, y + 1, true);
    drawCell(x + 2, y + 1, true);
    drawCell(x + 3, y + 1, true);
    drawCell(x + 1, y + 2, true);
    drawCell(x + 3, y + 2, true);
  } else if (game == GAME_BREAKOUT) {
    drawCell(x + 0, y + 0, true);
    drawCell(x + 1, y + 0, true);
    drawCell(x + 2, y + 0, true);
    drawCell(x + 3, y + 0, true);
    drawCell(x + 1, y + 1, true);
    drawCell(x + 0, y + 2, true);
    drawCell(x + 1, y + 2, true);
    drawCell(x + 2, y + 2, true);
    drawCell(x + 3, y + 2, true);
  } else {
    drawCell(x + 0, y + 0, true);
    drawCell(x + 2, y + 0, true);
    drawCell(x + 0, y + 1, true);
    drawCell(x + 1, y + 1, true);
    drawCell(x + 2, y + 1, true);
    drawCell(x + 1, y + 2, true);
  }
}

void handleMenuInput(uint8_t cmd) {
  if ((long)(millis() - menuInputLockedUntil) < 0) {
    return;
  }

  if (cmd == IR_LEFT || cmd == IR_UP) {
    selectedGame = (GameId)((selectedGame + GAME_COUNT - 1) % GAME_COUNT);
    drawMenu();
    delay(120);
  } else if (cmd == IR_RIGHT || cmd == IR_DOWN) {
    selectedGame = (GameId)((selectedGame + 1) % GAME_COUNT);
    drawMenu();
    delay(120);
  } else if (cmd == IR_ENTER) {
    startSelectedGame();
  }
}

void startSelectedGame() {
  activeGame = selectedGame;
  appMode = MODE_PLAYING;

  if (activeGame == GAME_SNAKE) {
    resetSnake();
  } else if (activeGame == GAME_TETRIS) {
    resetTetris();
  } else if (activeGame == GAME_FROGGER) {
    resetFrogger();
  } else if (activeGame == GAME_BOMBERMAN) {
    resetBomberman();
  } else if (activeGame == GAME_BREAKOUT) {
    resetBreakout();
  } else {
    resetInvaders();
  }
}

void showGameOver(int score, bool win) {
  appMode = MODE_GAME_OVER;
  lastScore = score;
  lastWin = win;
  gameOverShownAt = millis();

  clearScreen();
  drawCenteredText(win ? "WIN" : "LOSE", 6);
  drawCenteredNumber(score, 13);
  presentScreen();
}

void handleGameOverInput(uint8_t cmd) {
  if (millis() - gameOverShownAt < 250) {
    return;
  }

  if (cmd == IR_ENTER) {
    appMode = MODE_MENU;
    menuInputLockedUntil = millis() + 700;
    drawMenu();
  } else if (cmd == IR_LEFT || cmd == IR_RIGHT || cmd == IR_UP ||
             cmd == IR_DOWN || cmd == IR_DISC) {
    appMode = MODE_MENU;
    menuInputLockedUntil = millis() + 700;
    drawMenu();
  }
}

// ---------------- Snake ----------------

#define SNAKE_MAX_LENGTH 100
#define INITIAL_MOVE_INTERVAL 200
#define MIN_MOVE_INTERVAL 60
#define SPEED_DECREASE_PER_APPLE 10

int snakeX[SNAKE_MAX_LENGTH] = {5, 4, 3};
int snakeY[SNAKE_MAX_LENGTH] = {5, 5, 5};
int snakeLength = 3;
int snakeDx = 1;
int snakeDy = 0;
int lastSnakeDx = 1;
int lastSnakeDy = 0;
int appleX = 10;
int appleY = 5;
unsigned long lastSnakeMoveTime = 0;
unsigned long snakeMoveInterval = INITIAL_MOVE_INTERVAL;

void resetSnake() {
  snakeLength = 3;
  snakeX[0] = 5; snakeY[0] = 5;
  snakeX[1] = 4; snakeY[1] = 5;
  snakeX[2] = 3; snakeY[2] = 5;
  snakeDx = 1;
  snakeDy = 0;
  lastSnakeDx = snakeDx;
  lastSnakeDy = snakeDy;
  snakeMoveInterval = INITIAL_MOVE_INTERVAL;
  placeApple();
  clearScreen();
  drawInitialSnake();
  lastSnakeMoveTime = millis();
}

void handleSnakeInput(uint8_t cmd) {
  if (cmd == IR_UP && lastSnakeDy != 1) {
    snakeDx = 0; snakeDy = -1;
  } else if (cmd == IR_DOWN && lastSnakeDy != -1) {
    snakeDx = 0; snakeDy = 1;
  } else if (cmd == IR_LEFT && lastSnakeDx != 1) {
    snakeDx = -1; snakeDy = 0;
  } else if (cmd == IR_RIGHT && lastSnakeDx != -1) {
    snakeDx = 1; snakeDy = 0;
  }
}

void updateSnake() {
  if (millis() - lastSnakeMoveTime <= snakeMoveInterval) {
    return;
  }
  lastSnakeMoveTime = millis();

  int newHeadX = snakeX[0] + snakeDx;
  int newHeadY = snakeY[0] + snakeDy;

  if (newHeadX < 0 || newHeadX >= GAME_COLS ||
      newHeadY < 0 || newHeadY >= GAME_ROWS) {
    spiralFill();
    showGameOver(snakeLength - 3, false);
    return;
  }

  for (int i = 0; i < snakeLength; i++) {
    if (snakeX[i] == newHeadX && snakeY[i] == newHeadY) {
      spiralFill();
      showGameOver(snakeLength - 3, false);
      return;
    }
  }

  int tailX = snakeX[snakeLength - 1];
  int tailY = snakeY[snakeLength - 1];
  bool willGrow = (newHeadX == appleX && newHeadY == appleY);

  for (int i = snakeLength - 1; i > 0; i--) {
    snakeX[i] = snakeX[i - 1];
    snakeY[i] = snakeY[i - 1];
  }

  snakeX[0] = newHeadX;
  snakeY[0] = newHeadY;

  if (willGrow && snakeLength < SNAKE_MAX_LENGTH) {
    snakeLength++;
    placeApple();
    drawApple();
    if (snakeMoveInterval > MIN_MOVE_INTERVAL) {
      snakeMoveInterval -= SPEED_DECREASE_PER_APPLE;
    }
  } else {
    drawCell(tailX, tailY, false);
  }

  drawCell(newHeadX, newHeadY, true);
  lastSnakeDx = snakeDx;
  lastSnakeDy = snakeDy;
}

void placeApple() {
  bool valid = false;
  while (!valid) {
    appleX = random(0, GAME_COLS);
    appleY = random(0, GAME_ROWS);
    valid = true;
    for (int i = 0; i < snakeLength; i++) {
      if (snakeX[i] == appleX && snakeY[i] == appleY) {
        valid = false;
        break;
      }
    }
  }
}

void drawInitialSnake() {
  for (int i = 0; i < snakeLength; i++) {
    drawCell(snakeX[i], snakeY[i], true);
  }
  drawApple();
  presentScreen();
}

void drawApple() {
  drawCell(appleX, appleY, true);
}

void spiralFill() {
  int top = 0, bottom = GAME_ROWS - 1;
  int left = 0, right = GAME_COLS - 1;

  while (top <= bottom && left <= right) {
    for (int x = left; x <= right; x++) {
      drawCell(x, top, true);
      delay(4);
    }
    top++;

    for (int y = top; y <= bottom; y++) {
      drawCell(right, y, true);
      delay(4);
    }
    right--;

    if (top <= bottom) {
      for (int x = right; x >= left; x--) {
        drawCell(x, bottom, true);
        delay(4);
      }
      bottom--;
    }

    if (left <= right) {
      for (int y = bottom; y >= top; y--) {
        drawCell(left, y, true);
        delay(4);
      }
      left++;
    }
  }
}

// ---------------- Tetris ----------------

#define TETRIS_W 10
#define TETRIS_H 20
#define TETRIS_SCREEN_X 4
#define TETRIS_SCREEN_Y 2

bool tetrisBoard[TETRIS_H][TETRIS_W];
int tetrisPiece = 0;
int tetrisRot = 0;
int tetrisX = 3;
int tetrisY = 0;
int tetrisLines = 0;
unsigned long lastTetrisTick = 0;
unsigned long tetrisInterval = 420;

bool tetrisBlockAt(int piece, int rot, int bx, int by) {
  rot &= 3;

  if (piece == 0) { // I
    return rot % 2 == 0 ? (by == 1 && bx >= 0 && bx < 4)
                        : (bx == 2 && by >= 0 && by < 4);
  }
  if (piece == 1) { // O
    return bx >= 1 && bx <= 2 && by >= 1 && by <= 2;
  }
  if (piece == 2) { // T
    if (rot == 0) return ((by == 1 && bx >= 0 && bx < 3) || (bx == 1 && by == 2));
    if (rot == 1) return ((bx == 1 && by >= 0 && by < 3) || (bx == 2 && by == 1));
    if (rot == 2) return ((by == 1 && bx >= 0 && bx < 3) || (bx == 1 && by == 0));
    return ((bx == 1 && by >= 0 && by < 3) || (bx == 0 && by == 1));
  }
  if (piece == 3) { // L
    if (rot == 0) return ((bx == 0 && by >= 0 && by < 3) || (bx == 1 && by == 2));
    if (rot == 1) return ((by == 1 && bx >= 0 && bx < 3) || (bx == 0 && by == 2));
    if (rot == 2) return ((bx == 1 && by >= 0 && by < 3) || (bx == 0 && by == 0));
    return ((by == 1 && bx >= 0 && bx < 3) || (bx == 2 && by == 0));
  }
  if (piece == 4) { // J
    if (rot == 0) return ((bx == 1 && by >= 0 && by < 3) || (bx == 0 && by == 2));
    if (rot == 1) return ((by == 1 && bx >= 0 && bx < 3) || (bx == 0 && by == 0));
    if (rot == 2) return ((bx == 0 && by >= 0 && by < 3) || (bx == 1 && by == 0));
    return ((by == 1 && bx >= 0 && bx < 3) || (bx == 2 && by == 2));
  }
  if (piece == 5) { // S
    return rot % 2 == 0 ? ((by == 1 && bx >= 1 && bx <= 2) || (by == 2 && bx >= 0 && bx <= 1))
                        : ((bx == 1 && by >= 0 && by <= 1) || (bx == 2 && by >= 1 && by <= 2));
  }

  // Z
  return rot % 2 == 0 ? ((by == 1 && bx >= 0 && bx <= 1) || (by == 2 && bx >= 1 && bx <= 2))
                      : ((bx == 2 && by >= 0 && by <= 1) || (bx == 1 && by >= 1 && by <= 2));
}

bool tetrisCanPlace(int piece, int rot, int px, int py) {
  for (int by = 0; by < 4; by++) {
    for (int bx = 0; bx < 4; bx++) {
      if (!tetrisBlockAt(piece, rot, bx, by)) continue;
      int x = px + bx;
      int y = py + by;
      if (x < 0 || x >= TETRIS_W || y < 0 || y >= TETRIS_H) return false;
      if (tetrisBoard[y][x]) return false;
    }
  }
  return true;
}

void spawnTetrisPiece() {
  tetrisPiece = random(0, 7);
  tetrisRot = 0;
  tetrisX = 3;
  tetrisY = 0;
  if (!tetrisCanPlace(tetrisPiece, tetrisRot, tetrisX, tetrisY)) {
    showGameOver(tetrisLines, false);
  }
}

void lockTetrisPiece() {
  for (int by = 0; by < 4; by++) {
    for (int bx = 0; bx < 4; bx++) {
      if (!tetrisBlockAt(tetrisPiece, tetrisRot, bx, by)) continue;
      int x = tetrisX + bx;
      int y = tetrisY + by;
      if (x >= 0 && x < TETRIS_W && y >= 0 && y < TETRIS_H) {
        tetrisBoard[y][x] = true;
      }
    }
  }

  for (int y = TETRIS_H - 1; y >= 0; y--) {
    bool full = true;
    for (int x = 0; x < TETRIS_W; x++) {
      if (!tetrisBoard[y][x]) {
        full = false;
        break;
      }
    }
    if (!full) continue;

    for (int yy = y; yy > 0; yy--) {
      for (int x = 0; x < TETRIS_W; x++) {
        tetrisBoard[yy][x] = tetrisBoard[yy - 1][x];
      }
    }
    for (int x = 0; x < TETRIS_W; x++) {
      tetrisBoard[0][x] = false;
    }
    tetrisLines++;
    if (tetrisInterval > 180) tetrisInterval -= 18;
    y++;
  }

  spawnTetrisPiece();
}

void resetTetris() {
  for (int y = 0; y < TETRIS_H; y++) {
    for (int x = 0; x < TETRIS_W; x++) {
      tetrisBoard[y][x] = false;
    }
  }
  tetrisLines = 0;
  tetrisInterval = 420;
  lastTetrisTick = millis();
  clearScreen();
  spawnTetrisPiece();
  drawTetris();
}

void handleTetrisInput(uint8_t cmd) {
  if (cmd == IR_RIGHT && tetrisCanPlace(tetrisPiece, tetrisRot, tetrisX - 1, tetrisY)) {
    tetrisX--;
  } else if (cmd == IR_LEFT && tetrisCanPlace(tetrisPiece, tetrisRot, tetrisX + 1, tetrisY)) {
    tetrisX++;
  } else if (cmd == IR_UP && tetrisCanPlace(tetrisPiece, tetrisRot + 3, tetrisX, tetrisY)) {
    tetrisRot = (tetrisRot + 3) & 3;
  } else if (cmd == IR_DOWN) {
    if (tetrisCanPlace(tetrisPiece, tetrisRot, tetrisX, tetrisY + 1)) {
      tetrisY++;
    } else {
      lockTetrisPiece();
    }
  } else if (cmd == IR_ENTER) {
    while (tetrisCanPlace(tetrisPiece, tetrisRot, tetrisX, tetrisY + 1)) {
      tetrisY++;
    }
    lockTetrisPiece();
  }
  if (appMode == MODE_PLAYING) drawTetris();
}

void updateTetris() {
  if (millis() - lastTetrisTick <= tetrisInterval) return;
  lastTetrisTick = millis();

  if (tetrisCanPlace(tetrisPiece, tetrisRot, tetrisX, tetrisY + 1)) {
    tetrisY++;
  } else {
    lockTetrisPiece();
  }

  if (appMode == MODE_PLAYING) drawTetris();
}

void drawTetrisCell(int boardX, int boardY, bool on) {
  int sx = TETRIS_SCREEN_X + boardY;
  int sy = TETRIS_SCREEN_Y + boardX;
  drawCell(sx, sy, on);
}

void drawTetris() {
  clearScreen();

  for (int y = 0; y < TETRIS_H; y++) {
    for (int x = 0; x < TETRIS_W; x++) {
      if (tetrisBoard[y][x]) drawTetrisCell(x, y, true);
    }
  }

  for (int by = 0; by < 4; by++) {
    for (int bx = 0; bx < 4; bx++) {
      if (tetrisBlockAt(tetrisPiece, tetrisRot, bx, by)) {
        drawTetrisCell(tetrisX + bx, tetrisY + by, true);
      }
    }
  }

  for (int x = TETRIS_SCREEN_X - 1; x <= TETRIS_SCREEN_X + TETRIS_H; x++) {
    drawCell(x, TETRIS_SCREEN_Y - 1, true);
    drawCell(x, TETRIS_SCREEN_Y + TETRIS_W, true);
  }
  for (int y = TETRIS_SCREEN_Y - 1; y <= TETRIS_SCREEN_Y + TETRIS_W; y++) {
    drawCell(TETRIS_SCREEN_X - 1, y, true);
    drawCell(TETRIS_SCREEN_X + TETRIS_H, y, true);
  }
  presentScreen();
}

// ---------------- Frogger ----------------

int frogX = 0;
int frogY = 7;
int frogScore = 0;
bool frogVisible = true;
int frogLaneOffset[GAME_COLS];
unsigned long lastFrogTick = 0;
unsigned long lastFrogBlink = 0;

int frogLaneSpeed(int x) {
  if (x == 2 || x == 9 || x == 17 || x == 25) return 1;
  if (x == 4 || x == 11 || x == 19) return -1;
  return 0;
}

bool frogStaticObstacleAt(int x, int y) {
  if (x == 6) return y == 2 || y == 5 || y == 10;
  if (x == 14) return y == 1 || y == 8 || y == 12;
  if (x == 21) return y == 4 || y == 7 || y == 11;
  return false;
}

bool frogDynamicObstacleAt(int x, int y) {
  int speed = frogLaneSpeed(x);
  if (speed == 0) return false;
  int local = (y - frogLaneOffset[x] + GAME_ROWS * 2) % 6;
  return local == 0;
}

bool frogObstacleAt(int x, int y) {
  return frogStaticObstacleAt(x, y) || frogDynamicObstacleAt(x, y);
}

void resetFrogger() {
  frogX = 0;
  frogY = 7;
  frogScore = 0;
  frogVisible = true;
  for (int x = 0; x < GAME_COLS; x++) {
    frogLaneOffset[x] = random(0, 6);
  }
  frogLaneOffset[2] = 0;
  frogLaneOffset[4] = 4;
  lastFrogTick = millis();
  lastFrogBlink = millis();
  drawFrogger();
}

void handleFroggerInput(uint8_t cmd) {
  int nx = frogX;
  int ny = frogY;
  if (cmd == IR_UP) ny--;
  else if (cmd == IR_DOWN) ny++;
  else if (cmd == IR_LEFT) nx--;
  else if (cmd == IR_RIGHT) nx++;
  else return;

  if (nx < 0) nx = 0;
  if (nx >= GAME_COLS) nx = GAME_COLS - 1;
  if (ny < 0) ny = 0;
  if (ny >= GAME_ROWS) ny = GAME_ROWS - 1;
  frogX = nx;
  frogY = ny;
  if (frogX > frogScore) frogScore = frogX;
  if (frogObstacleAt(frogX, frogY)) {
    showGameOver(frogScore, false);
  } else if (frogX == GAME_COLS - 1) {
    showGameOver(frogScore, true);
  } else {
    drawFrogger();
  }
}

void updateFrogger() {
  unsigned long now = millis();
  bool redraw = false;

  if (now - lastFrogTick > 210) {
    lastFrogTick = now;
    for (int x = 0; x < GAME_COLS; x++) {
      int speed = frogLaneSpeed(x);
      if (speed != 0) {
        frogLaneOffset[x] = (frogLaneOffset[x] + speed + GAME_ROWS) % GAME_ROWS;
      }
    }
    if (frogObstacleAt(frogX, frogY)) {
      showGameOver(frogScore, false);
      return;
    }
    redraw = true;
  }

  if (now - lastFrogBlink > 300) {
    lastFrogBlink = now;
    frogVisible = !frogVisible;
    redraw = true;
  }

  if (redraw) drawFrogger();
}

void drawFrogger() {
  clearScreen();

  for (int y = 0; y < GAME_ROWS; y++) {
    for (int x = 0; x < GAME_COLS; x++) {
      if (frogObstacleAt(x, y)) {
        drawCell(x, y, true);
      }
    }
  }

  for (int y = 0; y < GAME_ROWS; y += 3) {
    drawCell(GAME_COLS - 1, y, true);
  }

  if (frogVisible) {
    drawCell(frogX, frogY, true);
  }
  presentScreen();
}

// ---------------- Bomberman ----------------

#define MAX_BOMBS 5
#define BOMB_FUSE_MS 1800
#define BLAST_MS 360
#define BLAST_RADIUS 3

struct Bomb {
  bool active;
  bool exploding;
  int x;
  int y;
  unsigned long placedAt;
  unsigned long explodedAt;
};

Bomb bombs[MAX_BOMBS];
bool bomberSoft[GAME_ROWS][GAME_COLS];
int playerX = 1;
int playerY = 1;
int aiX = 26;
int aiY = 12;
int bomberScore = 0;
unsigned long lastBomberTick = 0;
unsigned long lastAiMove = 0;
bool playerBlink = true;

bool bomberHardWallAt(int x, int y) {
  if (x <= 0 || x >= GAME_COLS - 1 || y <= 0 || y >= GAME_ROWS - 1) return true;
  return (x % 4 == 0 && y % 3 == 0);
}

bool bomberBombAt(int x, int y) {
  for (int i = 0; i < MAX_BOMBS; i++) {
    if (bombs[i].active && !bombs[i].exploding && bombs[i].x == x && bombs[i].y == y) {
      return true;
    }
  }
  return false;
}

bool bomberPassable(int x, int y) {
  if (x < 0 || x >= GAME_COLS || y < 0 || y >= GAME_ROWS) return false;
  if (bomberHardWallAt(x, y)) return false;
  if (bomberSoft[y][x]) return false;
  if (bomberBombAt(x, y)) return false;
  return true;
}

bool blastCanTravelThrough(int x, int y) {
  if (bomberHardWallAt(x, y)) return false;
  return true;
}

bool inBombBlastLine(int x, int y, Bomb* bomb) {
  if (!bomb->active) return false;
  if (x == bomb->x && y == bomb->y) return true;
  if (x != bomb->x && y != bomb->y) return false;

  int dx = 0;
  int dy = 0;
  if (x > bomb->x) dx = 1;
  else if (x < bomb->x) dx = -1;
  else if (y > bomb->y) dy = 1;
  else dy = -1;

  int cx = bomb->x;
  int cy = bomb->y;
  for (int i = 0; i < BLAST_RADIUS; i++) {
    cx += dx;
    cy += dy;
    if (!blastCanTravelThrough(cx, cy)) return false;
    if (cx == x && cy == y) return true;
    if (bomberSoft[cy][cx]) return false;
  }
  return false;
}

bool cellInActiveBlast(int x, int y) {
  for (int i = 0; i < MAX_BOMBS; i++) {
    if (bombs[i].active && bombs[i].exploding && inBombBlastLine(x, y, &bombs[i])) {
      return true;
    }
  }
  return false;
}

bool cellDangerousSoon(int x, int y) {
  unsigned long now = millis();
  for (int i = 0; i < MAX_BOMBS; i++) {
    if (!bombs[i].active) continue;
    if (bombs[i].exploding && inBombBlastLine(x, y, &bombs[i])) return true;
    if (!bombs[i].exploding && now - bombs[i].placedAt > BOMB_FUSE_MS - 650 &&
        inBombBlastLine(x, y, &bombs[i])) {
      return true;
    }
  }
  return false;
}

void placeBomb(int x, int y) {
  if (bomberBombAt(x, y)) return;
  for (int i = 0; i < MAX_BOMBS; i++) {
    if (!bombs[i].active) {
      bombs[i].active = true;
      bombs[i].exploding = false;
      bombs[i].x = x;
      bombs[i].y = y;
      bombs[i].placedAt = millis();
      bombs[i].explodedAt = 0;
      return;
    }
  }
}

void explodeBomb(int index) {
  bombs[index].exploding = true;
  bombs[index].explodedAt = millis();

  for (int d = 0; d < 4; d++) {
    int dx = (d == 0) - (d == 1);
    int dy = (d == 2) - (d == 3);
    int cx = bombs[index].x;
    int cy = bombs[index].y;
    for (int i = 0; i < BLAST_RADIUS; i++) {
      cx += dx;
      cy += dy;
      if (!blastCanTravelThrough(cx, cy)) break;
      if (bomberSoft[cy][cx]) {
        bomberSoft[cy][cx] = false;
        bomberScore++;
        break;
      }
    }
  }
}

void updateBombs() {
  unsigned long now = millis();
  for (int i = 0; i < MAX_BOMBS; i++) {
    if (!bombs[i].active) continue;
    if (!bombs[i].exploding && now - bombs[i].placedAt >= BOMB_FUSE_MS) {
      explodeBomb(i);
    } else if (bombs[i].exploding && now - bombs[i].explodedAt >= BLAST_MS) {
      bombs[i].active = false;
      bombs[i].exploding = false;
    }
  }
}

int manhattan(int ax, int ay, int bx, int by) {
  int dx = ax > bx ? ax - bx : bx - ax;
  int dy = ay > by ? ay - by : by - ay;
  return dx + dy;
}

void moveAi() {
  if (manhattan(aiX, aiY, playerX, playerY) <= 4) {
    placeBomb(aiX, aiY);
  }

  int dirs[5][2] = {{0, 0}, {1, 0}, {-1, 0}, {0, 1}, {0, -1}};
  int best = 0;
  int bestScore = -9999;

  for (int i = 0; i < 5; i++) {
    int nx = aiX + dirs[i][0];
    int ny = aiY + dirs[i][1];
    if (!bomberPassable(nx, ny) && !(nx == aiX && ny == aiY)) continue;

    int score = 0;
    if (cellDangerousSoon(nx, ny)) {
      score -= 1000;
    }
    score -= manhattan(nx, ny, playerX, playerY) * 3;
    score += random(0, 5);

    if (cellDangerousSoon(aiX, aiY)) {
      score += manhattan(nx, ny, aiX, aiY) * 20;
    }

    if (score > bestScore) {
      bestScore = score;
      best = i;
    }
  }

  aiX += dirs[best][0];
  aiY += dirs[best][1];
}

void resetBomberman() {
  playerX = 1;
  playerY = 1;
  aiX = 26;
  aiY = 12;
  bomberScore = 0;
  playerBlink = true;
  for (int i = 0; i < MAX_BOMBS; i++) {
    bombs[i].active = false;
    bombs[i].exploding = false;
  }

  for (int y = 0; y < GAME_ROWS; y++) {
    for (int x = 0; x < GAME_COLS; x++) {
      bomberSoft[y][x] = false;
      if (!bomberHardWallAt(x, y) &&
          !((x < 4 && y < 4) || (x > GAME_COLS - 5 && y > GAME_ROWS - 5))) {
        bomberSoft[y][x] = ((x * 7 + y * 5) % 11 == 0) || ((x + y * 3) % 17 == 0);
      }
    }
  }

  lastBomberTick = millis();
  lastAiMove = millis();
  drawBomberman();
}

void handleBombermanInput(uint8_t cmd) {
  int nx = playerX;
  int ny = playerY;
  if (cmd == IR_UP) ny--;
  else if (cmd == IR_DOWN) ny++;
  else if (cmd == IR_LEFT) nx--;
  else if (cmd == IR_RIGHT) nx++;
  else if (cmd == IR_ENTER) {
    placeBomb(playerX, playerY);
    drawBomberman();
    return;
  } else {
    return;
  }

  if (bomberPassable(nx, ny)) {
    playerX = nx;
    playerY = ny;
  }
  drawBomberman();
}

void updateBomberman() {
  unsigned long now = millis();
  if (now - lastBomberTick < 100) return;
  lastBomberTick = now;
  playerBlink = !playerBlink;

  updateBombs();

  if (now - lastAiMove > 280) {
    lastAiMove = now;
    moveAi();
  }

  if (cellInActiveBlast(playerX, playerY) || (playerX == aiX && playerY == aiY)) {
    showGameOver(bomberScore, false);
    return;
  }
  if (cellInActiveBlast(aiX, aiY)) {
    showGameOver(bomberScore + 10, true);
    return;
  }

  drawBomberman();
}

void drawBlastForBomb(Bomb* bomb) {
  drawCell(bomb->x, bomb->y, true);
  for (int d = 0; d < 4; d++) {
    int dx = (d == 0) - (d == 1);
    int dy = (d == 2) - (d == 3);
    int cx = bomb->x;
    int cy = bomb->y;
    for (int i = 0; i < BLAST_RADIUS; i++) {
      cx += dx;
      cy += dy;
      if (!blastCanTravelThrough(cx, cy)) break;
      drawCell(cx, cy, true);
      if (bomberSoft[cy][cx]) break;
    }
  }
}

void drawBomberman() {
  clearScreen();

  for (int y = 0; y < GAME_ROWS; y++) {
    for (int x = 0; x < GAME_COLS; x++) {
      if (bomberHardWallAt(x, y) || bomberSoft[y][x]) {
        drawCell(x, y, true);
      }
    }
  }

  for (int i = 0; i < MAX_BOMBS; i++) {
    if (!bombs[i].active) continue;
    if (bombs[i].exploding) drawBlastForBomb(&bombs[i]);
    else drawCell(bombs[i].x, bombs[i].y, true);
  }

  drawCell(aiX, aiY, true);
  if (playerBlink) {
    drawCell(playerX, playerY, true);
  }
  presentScreen();
}

// ---------------- Breakout ----------------

#define BREAKOUT_BRICK_ROWS 7
#define BREAKOUT_PADDLE_WIDTH 5
#define BREAKOUT_PADDLE_STEP 2

bool breakoutBricks[BREAKOUT_BRICK_ROWS][GAME_COLS];
int breakoutPaddleX = 12;
int breakoutBallX = 14;
int breakoutBallY = 12;
int breakoutBallDx = 1;
int breakoutBallDy = -1;
int breakoutBricksLeft = 0;
int breakoutScore = 0;
bool breakoutLaunched = false;
unsigned long lastBreakoutTick = 0;
unsigned long breakoutInterval = 105;

bool breakoutGlyph(char c, uint8_t rows[3]) {
  for (int i = 0; i < 3; i++) rows[i] = 0;

  switch (c) {
    case 'A': rows[0] = 0b010; rows[1] = 0b101; rows[2] = 0b111; return true;
    case 'B': rows[0] = 0b110; rows[1] = 0b111; rows[2] = 0b110; return true;
    case 'C': rows[0] = 0b111; rows[1] = 0b100; rows[2] = 0b111; return true;
    case 'E': rows[0] = 0b111; rows[1] = 0b110; rows[2] = 0b111; return true;
    case 'I': rows[0] = 0b111; rows[1] = 0b010; rows[2] = 0b111; return true;
    case 'J': rows[0] = 0b001; rows[1] = 0b001; rows[2] = 0b110; return true;
    case 'R': rows[0] = 0b110; rows[1] = 0b111; rows[2] = 0b101; return true;
    case 'S': rows[0] = 0b111; rows[1] = 0b110; rows[2] = 0b111; return true;
    case 'U': rows[0] = 0b101; rows[1] = 0b101; rows[2] = 0b111; return true;
    case 'Y': rows[0] = 0b101; rows[1] = 0b010; rows[2] = 0b010; return true;
  }
  return false;
}

void addBreakoutTextBricks(const char* text, int x, int y) {
  while (*text) {
    uint8_t rows[3];
    breakoutGlyph(*text++, rows);
    for (int row = 0; row < 3; row++) {
      for (int col = 0; col < 3; col++) {
        if ((rows[row] & (1 << (2 - col))) &&
            x + col >= 0 && x + col < GAME_COLS &&
            y + row >= 0 && y + row < BREAKOUT_BRICK_ROWS) {
          breakoutBricks[y + row][x + col] = true;
        }
      }
    }
    x += 4;
  }
}

void resetBreakout() {
  breakoutPaddleX = (GAME_COLS - BREAKOUT_PADDLE_WIDTH) / 2;
  breakoutBallX = breakoutPaddleX + BREAKOUT_PADDLE_WIDTH / 2;
  breakoutBallY = GAME_ROWS - 2;
  breakoutBallDx = 1;
  breakoutBallDy = -1;
  breakoutScore = 0;
  breakoutBricksLeft = 0;
  breakoutLaunched = false;
  breakoutInterval = 105;

  for (int y = 0; y < BREAKOUT_BRICK_ROWS; y++) {
    for (int x = 0; x < GAME_COLS; x++) {
      breakoutBricks[y][x] = false;
    }
  }

  addBreakoutTextBricks("CYBER", (GAME_COLS - textWidth("CYBER")) / 2, 0);
  addBreakoutTextBricks("CIRUJAS", (GAME_COLS - textWidth("CIRUJAS")) / 2, 4);

  for (int y = 0; y < BREAKOUT_BRICK_ROWS; y++) {
    for (int x = 0; x < GAME_COLS; x++) {
      if (breakoutBricks[y][x]) breakoutBricksLeft++;
    }
  }

  lastBreakoutTick = millis();
  drawBreakout();
}

void handleBreakoutInput(uint8_t cmd) {
  if (cmd == IR_LEFT && breakoutPaddleX > 0) {
    breakoutPaddleX -= BREAKOUT_PADDLE_STEP;
    if (breakoutPaddleX < 0) breakoutPaddleX = 0;
  } else if (cmd == IR_RIGHT && breakoutPaddleX < GAME_COLS - BREAKOUT_PADDLE_WIDTH) {
    breakoutPaddleX += BREAKOUT_PADDLE_STEP;
    if (breakoutPaddleX > GAME_COLS - BREAKOUT_PADDLE_WIDTH) {
      breakoutPaddleX = GAME_COLS - BREAKOUT_PADDLE_WIDTH;
    }
  } else if (cmd == IR_ENTER) {
    breakoutLaunched = true;
  } else {
    return;
  }

  if (!breakoutLaunched) {
    breakoutBallX = breakoutPaddleX + BREAKOUT_PADDLE_WIDTH / 2;
  }
  drawBreakout();
}

void updateBreakout() {
  if (!breakoutLaunched || millis() - lastBreakoutTick < breakoutInterval) return;
  lastBreakoutTick = millis();

  int nextX = breakoutBallX + breakoutBallDx;
  int nextY = breakoutBallY + breakoutBallDy;

  if (nextX < 0 || nextX >= GAME_COLS) {
    breakoutBallDx = -breakoutBallDx;
    nextX = breakoutBallX + breakoutBallDx;
  }
  if (nextY < 0) {
    breakoutBallDy = 1;
    nextY = breakoutBallY + breakoutBallDy;
  }

  if (nextY >= 0 && nextY < BREAKOUT_BRICK_ROWS && breakoutBricks[nextY][nextX]) {
    breakoutBricks[nextY][nextX] = false;
    breakoutBricksLeft--;
    breakoutScore++;
    if (breakoutInterval > 55 && breakoutScore % 8 == 0) breakoutInterval -= 5;
    breakoutBallDy = -breakoutBallDy;
    nextY = breakoutBallY + breakoutBallDy;
  }

  if (breakoutBallDy > 0 && nextY == GAME_ROWS - 1 &&
      nextX >= breakoutPaddleX && nextX < breakoutPaddleX + BREAKOUT_PADDLE_WIDTH) {
    int hit = nextX - breakoutPaddleX;
    if (hit <= 1) breakoutBallDx = -1;
    else if (hit >= BREAKOUT_PADDLE_WIDTH - 2) breakoutBallDx = 1;
    breakoutBallDy = -1;
    nextY = breakoutBallY;
  }

  if (nextY >= GAME_ROWS) {
    showGameOver(breakoutScore, false);
    return;
  }

  breakoutBallX = nextX;
  breakoutBallY = nextY;
  if (breakoutBricksLeft == 0) {
    showGameOver(breakoutScore, true);
    return;
  }
  drawBreakout();
}

void drawBreakout() {
  clearScreen();
  for (int y = 0; y < BREAKOUT_BRICK_ROWS; y++) {
    for (int x = 0; x < GAME_COLS; x++) {
      if (breakoutBricks[y][x]) drawCell(x, y, true);
    }
  }
  for (int x = 0; x < BREAKOUT_PADDLE_WIDTH; x++) {
    drawCell(breakoutPaddleX + x, GAME_ROWS - 1, true);
  }
  drawCell(breakoutBallX, breakoutBallY, true);
  presentScreen();
}

// ---------------- Space Invaders ----------------

#define INVADER_ROWS 1
#define INVADER_COLS 5

bool invaders[INVADER_ROWS][INVADER_COLS];
int invaderFleetX = 2;
int invaderFleetY = 1;
int invaderFleetDx = 1;
int invaderShipX = 13;
int invaderScore = 0;
bool playerShotActive = false;
int playerShotX = 0;
int playerShotY = 0;
bool invaderShotActive = false;
int invaderShotX = 0;
int invaderShotY = 0;
unsigned long lastInvaderMove = 0;
unsigned long lastPlayerShotMove = 0;
unsigned long lastInvaderShotAt = 0;
unsigned long lastInvaderShotMove = 0;
unsigned long invaderMoveInterval = 300;

int invaderCount() {
  int count = 0;
  for (int row = 0; row < INVADER_ROWS; row++) {
    for (int col = 0; col < INVADER_COLS; col++) {
      if (invaders[row][col]) count++;
    }
  }
  return count;
}

bool invaderHitAt(int x, int y) {
  for (int row = 0; row < INVADER_ROWS; row++) {
    for (int col = 0; col < INVADER_COLS; col++) {
      if (!invaders[row][col]) continue;
      int left = invaderFleetX + col * 5;
      int top = invaderFleetY + row * 3;
      if (x >= left && x <= left + 2 && y >= top && y <= top + 1) {
        invaders[row][col] = false;
        return true;
      }
    }
  }
  return false;
}

void fireInvaderShot() {
  int count = invaderCount();
  if (count == 0) return;
  int target = random(0, count);
  for (int row = 0; row < INVADER_ROWS; row++) {
    for (int col = 0; col < INVADER_COLS; col++) {
      if (!invaders[row][col]) continue;
      if (target-- == 0) {
        invaderShotActive = true;
        invaderShotX = invaderFleetX + col * 5 + 1;
        invaderShotY = invaderFleetY + row * 3 + 2;
        return;
      }
    }
  }
}

void resetInvaders() {
  for (int row = 0; row < INVADER_ROWS; row++) {
    for (int col = 0; col < INVADER_COLS; col++) {
      invaders[row][col] = true;
    }
  }
  invaderFleetX = 2;
  invaderFleetY = 1;
  invaderFleetDx = 1;
  invaderShipX = (GAME_COLS - 3) / 2;
  invaderScore = 0;
  playerShotActive = false;
  invaderShotActive = false;
  invaderMoveInterval = 300;
  lastInvaderMove = millis();
  lastPlayerShotMove = millis();
  lastInvaderShotAt = millis();
  lastInvaderShotMove = millis();
  drawInvaders();
}

void handleInvadersInput(uint8_t cmd) {
  if (cmd == IR_LEFT && invaderShipX > 0) {
    invaderShipX--;
  } else if (cmd == IR_RIGHT && invaderShipX < GAME_COLS - 3) {
    invaderShipX++;
  } else if (cmd == IR_ENTER && !playerShotActive) {
    playerShotActive = true;
    playerShotX = invaderShipX + 1;
    playerShotY = GAME_ROWS - 3;
    lastPlayerShotMove = millis();
  } else {
    return;
  }
  drawInvaders();
}

void updateInvaders() {
  unsigned long now = millis();
  bool redraw = false;

  if (playerShotActive && now - lastPlayerShotMove >= 70) {
    lastPlayerShotMove = now;
    playerShotY--;
    if (playerShotY < 0) {
      playerShotActive = false;
    } else if (invaderHitAt(playerShotX, playerShotY)) {
      playerShotActive = false;
      invaderScore += 10;
      if (invaderMoveInterval > 115) invaderMoveInterval -= 10;
      if (invaderCount() == 0) {
        showGameOver(invaderScore, true);
        return;
      }
    }
    redraw = true;
  }

  if (now - lastInvaderMove >= invaderMoveInterval) {
    lastInvaderMove = now;
    if (invaderFleetX + invaderFleetDx < 0 || invaderFleetX + invaderFleetDx > 5) {
      invaderFleetDx = -invaderFleetDx;
      invaderFleetY++;
      if (invaderFleetY + (INVADER_ROWS - 1) * 3 + 1 >= GAME_ROWS - 2) {
        showGameOver(invaderScore, false);
        return;
      }
    } else {
      invaderFleetX += invaderFleetDx;
    }
    redraw = true;
  }

  if (!invaderShotActive && now - lastInvaderShotAt >= 850) {
    lastInvaderShotAt = now;
    fireInvaderShot();
    redraw = true;
  }

  if (invaderShotActive && now - lastInvaderShotMove >= 105) {
    lastInvaderShotMove = now;
    invaderShotY++;
    if (invaderShotY >= GAME_ROWS) {
      invaderShotActive = false;
    } else if (invaderShotY >= GAME_ROWS - 2 &&
               invaderShotX >= invaderShipX && invaderShotX < invaderShipX + 3) {
      showGameOver(invaderScore, false);
      return;
    }
    redraw = true;
  }

  if (redraw) drawInvaders();
}

void drawInvaders() {
  clearScreen();
  for (int row = 0; row < INVADER_ROWS; row++) {
    for (int col = 0; col < INVADER_COLS; col++) {
      if (!invaders[row][col]) continue;
      int x = invaderFleetX + col * 5;
      int y = invaderFleetY + row * 3;
      drawCell(x, y, true);
      drawCell(x + 2, y, true);
      drawCell(x, y + 1, true);
      drawCell(x + 1, y + 1, true);
      drawCell(x + 2, y + 1, true);
    }
  }

  drawCell(invaderShipX + 1, GAME_ROWS - 2, true);
  drawCell(invaderShipX, GAME_ROWS - 1, true);
  drawCell(invaderShipX + 1, GAME_ROWS - 1, true);
  drawCell(invaderShipX + 2, GAME_ROWS - 1, true);
  if (playerShotActive) drawCell(playerShotX, playerShotY, true);
  if (invaderShotActive) drawCell(invaderShotX, invaderShotY, true);
  presentScreen();
}
