#pragma once

#include <Arduino.h>
#include <Adafruit_ST7789.h>
#include "graphics.h"

struct ScreenLayoutContext {
  Graphics& gfx;
  Adafruit_ST7789& tft;
  uint8_t titleIndent;
  uint8_t measurementIndent;
  uint8_t blockIndent;
};
