#pragma once
#include <Arduino.h>
#include <Adafruit_ST7789.h>
#include "config.h"

struct IconRun;

class Graphics {
  public:
    Graphics(Adafruit_ST7789& display)
      : tft(display) {}

    uint16_t getTextWidth(const char* text, uint8_t textSize = 1);
    void drawSnowflake(int x, int y, float size, int depth);
    void drawLine(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void drawIcon(const uint8_t icon[19][4], uint8_t x, uint16_t y, uint16_t color, uint8_t scale = 6);
    void drawOilPressureWarningIcon(int16_t x, int16_t y, uint16_t color);
    void drawAbsWarningIcon(int16_t x, int16_t y, uint16_t color);
    void drawBrakeWarningIcon(int16_t x, int16_t y, uint16_t color);
    void drawHandbrakeWarningIcon(int16_t x, int16_t y, uint16_t color);
    void drawCoolantWarningIcon(int16_t x, int16_t y, uint16_t color);
    void drawAirbagWarningIcon(int16_t x, int16_t y, uint16_t color);
    void drawFuelWarningIcon(int16_t x, int16_t y, uint16_t color);
    void drawWashWarningIcon(int16_t x, int16_t y, uint16_t color);
    void drawDoorWarningIcon(int16_t x, int16_t y, uint16_t color);
    void drawWarningText(const char* text, int16_t y, uint16_t color, int size = 1);
    void clearTextOverBackground(int16_t x, int16_t y, uint16_t w, uint16_t h, int16_t left_padding = 4, uint8_t right_padding = 4);
    void drawHeaderSeparator();
    void drawScreenBackgroundKeepingHeaderSeparator(bool& headerSeparatorDrawn);
    void drawScreenFromStrip(uint16_t startY = 0, uint16_t endY = 320);
    void drawText(const char* text, int16_t x, int16_t y, uint16_t color, uint8_t max_width = 0, int size = 1);
    void drawRightAlignedText(const char* text, int16_t rightX, int16_t y, uint16_t color, uint8_t max_width = 0, int size = 1);

  private:
    Adafruit_ST7789& tft;
    void drawWarningIconRuns(
      int16_t x,
      int16_t y,
      uint16_t color,
      const IconRun* edgeRuns,
      uint16_t edgeRunCount,
      const IconRun* runs,
      uint16_t runCount);
};