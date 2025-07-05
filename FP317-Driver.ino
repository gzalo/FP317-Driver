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

FP317_gfx* gfx;

void setup() {
  // put your setup code here, to run once:
  gfx = new FP317_gfx();
  //driver->clearDisplay();
  delay(1000);
}

void loop() {
  gfx->setFont(&TomThumb);
  gfx->setCursor(0,5);
  gfx->setLag(0);
  gfx->clearDisplay();
  gfx->print("CYBERCIRUJAS");
  delay(3000);
  gfx->clearDisplay();
  delay(2000);
}
