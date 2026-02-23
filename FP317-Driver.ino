/*
 * File: FP317.ino
 * Implements the FP317_gfx class and draws some stuff to play around
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

// Snake state
int snakeX[100] = {5, 4, 3};
int snakeY[100] = {5, 5, 5};
int snakeLength = 3;

int dx = 1;  // initial direction (right)
int dy = 0;

// Apple
int appleX = 10;
int appleY = 5;

// Last direction for input filtering
int lastDx = dx;
int lastDy = dy;

// IR codes
#define IR_UP    0x58
#define IR_DOWN  0x59
#define IR_LEFT  0x5A
#define IR_RIGHT 0x5B
#define IR_ENTER 0x5C
#define IR_DISC  0x3F


unsigned long lastMoveTime = 0;
unsigned long moveInterval = 200; // ms
#define INITIAL_MOVE_INTERVAL 200
#define MIN_MOVE_INTERVAL 60
#define SPEED_DECREASE_PER_APPLE 10
bool gameRunning = true;

void setup() {
  Serial.begin(115200);
  gfx = new FP317_gfx();
  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);
  gfx->clearDisplay();
  delay(1000);
  drawInitialGame();
}

void resetGame() {
  gameRunning = true;

  // Reset snake
  snakeLength = 3;
  snakeX[0] = 5; snakeY[0] = 5;
  snakeX[1] = 4; snakeY[1] = 5;
  snakeX[2] = 3; snakeY[2] = 5;
  dx = 1;
  dy = 0;
  lastDx = dx;
  lastDy = dy;

  // Reset apple
  placeApple();

  // Clear and draw fresh
  gfx->clearDisplay();
  drawInitialGame();

  moveInterval = INITIAL_MOVE_INTERVAL;
  lastMoveTime = millis();
}

void loop() {
  // IR input
  if (IrReceiver.decode()) {
    uint8_t cmd = IrReceiver.decodedIRData.command;
    Serial.print("IR code: 0x");
    Serial.println(cmd, HEX);

    if (!gameRunning) {
      if (cmd == IR_UP || cmd == IR_DOWN || cmd == IR_LEFT || cmd == IR_RIGHT || cmd == IR_ENTER || cmd == IR_DISC) {
        resetGame();
      }
      IrReceiver.resume();
      return;
    }

    if (cmd == IR_UP && lastDy != 1)      { dx = 0; dy = -1; }
    else if (cmd == IR_DOWN && lastDy != -1)  { dx = 0; dy = 1; }
    else if (cmd == IR_LEFT && lastDx != 1)   { dx = -1; dy = 0; }
    else if (cmd == IR_RIGHT && lastDx != -1) { dx = 1; dy = 0; }
    IrReceiver.resume();
  }

  if(!gameRunning){
    return;
  }

  /*gfx->setFont(&TomThumb);
  gfx->setCursor(0,5);
  gfx->setLag(0);
  gfx->clearDisplay();
  gfx->print("CYBERCIRUJAS");
  delay(3000);
  gfx->clearDisplay();
  delay(2000);*/

    if (millis() - lastMoveTime > moveInterval) {
    lastMoveTime = millis();

    int newHeadX = snakeX[0] + dx;
    int newHeadY = snakeY[0] + dy;

    // Check collision with walls
    if (newHeadX < 0 || newHeadX >= GAME_COLS || newHeadY < 0 || newHeadY >= GAME_ROWS) {
      gameOver();
      return;
    }
    
    // Check collision with self
    for (int i = 0; i < snakeLength; i++) {
      if (snakeX[i] == newHeadX && snakeY[i] == newHeadY) {
        gameOver();
        return;
      }
    }

    // FIXED: Store tail position BEFORE shifting the body
    int tailX = snakeX[snakeLength - 1];
    int tailY = snakeY[snakeLength - 1];

    // Check if snake will eat apple
    bool willGrow = (newHeadX == appleX && newHeadY == appleY);

    // Shift body segments
    for (int i = snakeLength - 1; i > 0; i--) {
      snakeX[i] = snakeX[i - 1];
      snakeY[i] = snakeY[i - 1];
    }
    
    // Add new head
    snakeX[0] = newHeadX;
    snakeY[0] = newHeadY;

    // Handle apple eating
    if (willGrow) {
      snakeLength++;
      placeApple();
      drawApple();
      if (moveInterval > MIN_MOVE_INTERVAL) {
        moveInterval -= SPEED_DECREASE_PER_APPLE;
      }
    }
    drawCell(tailX, tailY, false); // erase tail only if not growing

    // FIXED: Always draw new head last to ensure it's visible
    drawCell(newHeadX, newHeadY, true);

    // Update direction tracking
    lastDx = dx;
    lastDy = dy;
  }
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

void drawInitialGame() {
  for (int i = 0; i < snakeLength; i++) {
    drawCell(snakeX[i], snakeY[i], true);
  }
  drawApple();
}

void drawCell(int x, int y, bool on) {
  gfx->fillRect(x, y, 1, 1, on);
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
      delay(5);
    }
    top++;

    for (int y = top; y <= bottom; y++) {
      drawCell(right, y, true);
      delay(5);
    }
    right--;

    if (top <= bottom) {
      for (int x = right; x >= left; x--) {
        drawCell(x, bottom, true);
        delay(5);
      }
      bottom--;
    }

    if (left <= right) {
      for (int y = bottom; y >= top; y--) {
        drawCell(left, y, true);
        delay(5);
      }
      left++;
    }
  }
}

void gameOver() {
  gameRunning = false;

  spiralFill();
  delay(500);

  gfx->clearDisplay();
  gfx->setFont(&TomThumb);
  gfx->setLag(0);

  gfx->setCursor(0, 5);
  gfx->print(snakeLength - 3);
  delay(1000);

  gfx->setCursor(0, 12);
  gfx->print("puntos");
  delay(2000);
}
