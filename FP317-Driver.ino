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
#define IR_UP    0x3
#define IR_DOWN  0x2
#define IR_LEFT  0xE
#define IR_RIGHT 0x1A


unsigned long lastMoveTime = 0;
unsigned long moveInterval = 200; // ms
bool gameRunning = true;

void setup() {
  // put your setup code here, to run once:
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

  lastMoveTime = millis();
}

void loop() {
  // IR input
  if (IrReceiver.decode()) {
    uint8_t cmd = IrReceiver.decodedIRData.command;

    if (!gameRunning) {
      if (cmd == IR_UP || cmd == IR_DOWN || cmd == IR_LEFT || cmd == IR_RIGHT) {
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

void gameOver() {
  gameRunning = false;

  gfx->clearDisplay();
  gfx->setFont(&TomThumb);
  gfx->setCursor(0,5);
  gfx->setLag(0);
  gfx->clearDisplay();
  gfx->print("LOSER!");

}
