#ifndef SIM_ADAFRUIT_GFX_H
#define SIM_ADAFRUIT_GFX_H

#include "Arduino.h"

typedef struct {
  uint16_t bitmapOffset;
  uint8_t width;
  uint8_t height;
  uint8_t xAdvance;
  int8_t xOffset;
  int8_t yOffset;
} GFXglyph;

typedef struct {
  uint8_t* bitmap;
  GFXglyph* glyph;
  uint8_t first;
  uint8_t last;
  uint8_t yAdvance;
} GFXfont;

class Adafruit_GFX {
 public:
  int16_t WIDTH;
  int16_t HEIGHT;
  int16_t _width;
  int16_t _height;

  Adafruit_GFX(int16_t w, int16_t h)
      : WIDTH(w), HEIGHT(h), _width(w), _height(h) {}

  virtual ~Adafruit_GFX() = default;
  virtual void drawPixel(int16_t x, int16_t y, uint16_t color) = 0;

  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    for (int16_t yy = y; yy < y + h; yy++) {
      for (int16_t xx = x; xx < x + w; xx++) {
        drawPixel(xx, yy, color);
      }
    }
  }

  void setFont(const GFXfont* font = nullptr) {
    currentFont = font;
  }

  void setCursor(int16_t x, int16_t y) {
    cursorX = x;
    cursorY = y;
  }

  void print(int value) {
    print(std::to_string(value).c_str());
  }

  void print(const char* text) {
    while (*text) {
      drawChar(*text++);
    }
  }

 private:
  int16_t cursorX = 0;
  int16_t cursorY = 0;
  const GFXfont* currentFont = nullptr;

  void drawChar(char c) {
    if (currentFont && currentFont->glyph && currentFont->bitmap &&
        c >= currentFont->first && c <= currentFont->last) {
      drawGfxChar(c);
      return;
    }

    if (c == ' ') {
      cursorX += 4;
      return;
    }

    uint8_t pattern[5] = {};
    if (c >= '0' && c <= '9') {
      static const uint8_t digits[10][5] = {
          {0b111, 0b101, 0b101, 0b101, 0b111},
          {0b010, 0b110, 0b010, 0b010, 0b111},
          {0b111, 0b001, 0b111, 0b100, 0b111},
          {0b111, 0b001, 0b111, 0b001, 0b111},
          {0b101, 0b101, 0b111, 0b001, 0b001},
          {0b111, 0b100, 0b111, 0b001, 0b111},
          {0b111, 0b100, 0b111, 0b101, 0b111},
          {0b111, 0b001, 0b001, 0b001, 0b001},
          {0b111, 0b101, 0b111, 0b101, 0b111},
          {0b111, 0b101, 0b111, 0b001, 0b111},
      };
      for (int i = 0; i < 5; i++) pattern[i] = digits[c - '0'][i];
    } else if (fallbackPattern(c, pattern)) {
      // Pattern filled below.
    } else {
      static const uint8_t fallback[5] = {0b111, 0b100, 0b110, 0b100, 0b111};
      for (int i = 0; i < 5; i++) pattern[i] = fallback[i];
    }

    int16_t top = cursorY - 5;
    for (int row = 0; row < 5; row++) {
      for (int col = 0; col < 3; col++) {
        if (pattern[row] & (1 << (2 - col))) {
          drawPixel(cursorX + col, top + row, 1);
        }
      }
    }
    cursorX += 4;
  }

  bool fallbackPattern(char c, uint8_t pattern[5]) {
    if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');

    uint8_t rows[5] = {};
    switch (c) {
      case 'a': rows[0] = 0b010; rows[1] = 0b101; rows[2] = 0b111; rows[3] = 0b101; rows[4] = 0b101; break;
      case 'e': rows[0] = 0b111; rows[1] = 0b100; rows[2] = 0b110; rows[3] = 0b100; rows[4] = 0b111; break;
      case 'i': rows[0] = 0b111; rows[1] = 0b010; rows[2] = 0b010; rows[3] = 0b010; rows[4] = 0b111; break;
      case 'n': rows[0] = 0b110; rows[1] = 0b101; rows[2] = 0b101; rows[3] = 0b101; rows[4] = 0b101; break;
      case 'o': rows[0] = 0b111; rows[1] = 0b101; rows[2] = 0b101; rows[3] = 0b101; rows[4] = 0b111; break;
      case 'p': rows[0] = 0b110; rows[1] = 0b101; rows[2] = 0b110; rows[3] = 0b100; rows[4] = 0b100; break;
      case 's': rows[0] = 0b111; rows[1] = 0b100; rows[2] = 0b111; rows[3] = 0b001; rows[4] = 0b111; break;
      case 't': rows[0] = 0b111; rows[1] = 0b010; rows[2] = 0b010; rows[3] = 0b010; rows[4] = 0b010; break;
      case 'u': rows[0] = 0b101; rows[1] = 0b101; rows[2] = 0b101; rows[3] = 0b101; rows[4] = 0b111; break;
      default: return false;
    }

    for (int i = 0; i < 5; i++) pattern[i] = rows[i];
    return true;
  }

  void drawGfxChar(char c) {
    const GFXglyph& glyph = currentFont->glyph[c - currentFont->first];
    uint16_t bitOffset = glyph.bitmapOffset;
    uint8_t bits = 0;
    uint8_t bit = 0;

    for (uint8_t yy = 0; yy < glyph.height; yy++) {
      for (uint8_t xx = 0; xx < glyph.width; xx++) {
        if (bit == 0) {
          bits = currentFont->bitmap[bitOffset++];
          bit = 0x80;
        }
        if (bits & bit) {
          drawPixel(cursorX + glyph.xOffset + xx, cursorY + glyph.yOffset + yy, 1);
        }
        bit >>= 1;
      }
    }

    cursorX += glyph.xAdvance;
  }
};

#endif
