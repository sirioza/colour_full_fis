#include "graphics.h"
#include <math.h>
#include "icons.h"
#include "background.h"
#include <U8g2_for_Adafruit_GFX.h>

extern U8G2_FOR_ADAFRUIT_GFX u8g2;

const uint8_t BACKGROUND_ROWS_PER_CHUNK = 10;
const uint8_t WARNING_RUN_SERVICE_CHECK_INTERVAL = 16;
const uint16_t HEADER_SEPARATOR_Y = 45;
const uint8_t HEADER_SEPARATOR_H = 2;
const uint16_t SCREEN_CONTENT_BOTTOM = 295;
static uint16_t backgroundBuffer[SCREEN_WIDTH * BACKGROUND_ROWS_PER_CHUNK];

uint16_t Graphics::getTextWidth(const char* text, uint8_t textSize) {
  int16_t x1, y1;
  uint16_t w, h;

  tft.setTextSize(textSize);
  tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

  return w;
}

void Graphics::drawSnowflake(int x, int y, float size, int depth) {
  (void)depth;

  struct SnowflakeLine {
    int8_t x0;
    int8_t y0;
    int8_t x1;
    int8_t y1;
  };

  static const SnowflakeLine snowflakeLines[] PROGMEM = {
    { 0, 0, 5, 0 },
    { 5, 0, 8, -2 },
    { 5, 0, 8, 2 },
    { 0, 0, 3, -4 },
    { 3, -4, 6, -5 },
    { 3, -4, 3, -7 },
    { 0, 0, -2, -4 },
    { -2, -4, -2, -7 },
    { -2, -4, -5, -5 },
    { 0, 0, -5, 0 },
    { -5, 0, -8, -2 },
    { -5, 0, -8, 2 },
    { 0, 0, -2, 4 },
    { -2, 4, -5, 5 },
    { -2, 4, -2, 7 },
    { 0, 0, 3, 4 },
    { 3, 4, 3, 7 },
    { 3, 4, 6, 5 },
  };

  int16_t scale = (int16_t)(size + 2.5f) / 5;
  if (scale < 1) {
    scale = 1;
  }

  for (uint8_t i = 0; i < sizeof(snowflakeLines) / sizeof(snowflakeLines[0]); i++) {
    SnowflakeLine line;
    memcpy_P(&line, &snowflakeLines[i], sizeof(line));
    tft.drawLine(
      x + line.x0 * scale,
      y + line.y0 * scale,
      x + line.x1 * scale,
      y + line.y1 * scale,
      ILI9341_WHITE
    );
  }

  tft.fillCircle(x, y, 2 * scale, ILI9341_WHITE);
}

void Graphics::drawLine(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color){
	for (int i = 0; i < h; ++i) {
		tft.drawFastHLine(x, y + i, w, color);
  }
}

void Graphics::drawIcon(const uint8_t icon[19][4], uint8_t x, uint16_t y, uint16_t color, uint8_t scale) {
  for (int r = 0; r < 19; r++) {
    int runStart = -1;

    for (int c = 0; c <= 32; c++) {
      bool isSet = false;
      if (c < 32) {
        uint8_t rowByte = pgm_read_byte(&icon[r][c / 8]);
        isSet = (rowByte & (0x80 >> (c % 8))) != 0;
      }

      if (isSet && runStart < 0) {
        runStart = c;
        continue;
      }

      if ((isSet || runStart < 0)) {
        continue;
      }

      uint8_t runWidth = c - runStart;
      int16_t drawX = x + runStart * scale;
      int16_t drawY = y + r * scale;

      if (scale == 1) {
        tft.drawFastHLine(drawX, drawY, runWidth, color);
      } else {
        tft.fillRect(drawX, drawY, runWidth * scale, scale, color);
      }

      runStart = -1;
    }
  }
}

void Graphics::drawWarningIconRuns(int16_t x, int16_t y, uint16_t color, const IconRun* edgeRuns, uint16_t edgeRunCount, const IconRun* runs, uint16_t runCount) {
  uint8_t r = ((color >> 11) & 0x1F) * 6 / 8;
  uint8_t g = ((color >> 5) & 0x3F) * 6 / 8;
  uint8_t b = (color & 0x1F) * 6 / 8;
  uint16_t edgeColor = (r << 11) | (g << 5) | b;

  for (uint16_t i = 0; i < edgeRunCount; i++) {
    uint8_t runY = pgm_read_byte(&edgeRuns[i].y);
    uint8_t runX = pgm_read_byte(&edgeRuns[i].x);
    uint8_t runW = pgm_read_byte(&edgeRuns[i].w);
    tft.drawFastHLine(x + runX, y + runY, runW, edgeColor);
  }

  for (uint16_t i = 0; i < runCount; i++) {
    uint8_t runY = pgm_read_byte(&runs[i].y);
    uint8_t runX = pgm_read_byte(&runs[i].x);
    uint8_t runW = pgm_read_byte(&runs[i].w);
    tft.drawFastHLine(x + runX, y + runY, runW, color);
  }
}

void Graphics::drawOilPressureWarningIcon(int16_t x, int16_t y, uint16_t color) {
  drawWarningIconRuns(x, y, color, oilPressureAvifEdgeRuns, OIL_PRESSURE_AVIF_EDGE_RUN_COUNT, oilPressureAvifRuns, OIL_PRESSURE_AVIF_RUN_COUNT);
}

void Graphics::drawAbsWarningIcon(int16_t x, int16_t y, uint16_t color) {
  drawWarningIconRuns(x, y, color, absWarningEdgeRuns, ABS_WARNING_EDGE_RUN_COUNT, absWarningRuns, ABS_WARNING_RUN_COUNT);
}

void Graphics::drawBrakeWarningIcon(int16_t x, int16_t y, uint16_t color) {
  drawWarningIconRuns(x, y, color, brakeWarningEdgeRuns, BRAKE_WARNING_EDGE_RUN_COUNT, brakeWarningRuns, BRAKE_WARNING_RUN_COUNT);
}

void Graphics::drawHandbrakeWarningIcon(int16_t x, int16_t y, uint16_t color) {
  drawWarningIconRuns(x, y, color, handbrakeWarningEdgeRuns, HANDBRAKE_WARNING_EDGE_RUN_COUNT, handbrakeWarningRuns, HANDBRAKE_WARNING_RUN_COUNT);
}

void Graphics::drawCoolantWarningIcon(int16_t x, int16_t y, uint16_t color) {
  drawWarningIconRuns(x, y, color, coolantWarningEdgeRuns, COOLANT_WARNING_EDGE_RUN_COUNT, coolantWarningRuns, COOLANT_WARNING_RUN_COUNT);
}

void Graphics::drawAirbagWarningIcon(int16_t x, int16_t y, uint16_t color) {
  drawWarningIconRuns(x, y, color, airbagWarningEdgeRuns, AIRBAG_WARNING_EDGE_RUN_COUNT, airbagWarningRuns, AIRBAG_WARNING_RUN_COUNT);
}

void Graphics::drawFuelWarningIcon(int16_t x, int16_t y, uint16_t color) {
  drawWarningIconRuns(x, y, color, fuelWarningEdgeRuns, FUEL_WARNING_EDGE_RUN_COUNT, fuelWarningRuns, FUEL_WARNING_RUN_COUNT);
}

void Graphics::drawWashWarningIcon(int16_t x, int16_t y, uint16_t color) {
  drawWarningIconRuns(x, y, color, washWarningEdgeRuns, WASH_WARNING_EDGE_RUN_COUNT, washWarningRuns, WASH_WARNING_RUN_COUNT);
}

void Graphics::drawDoorWarningIcon(int16_t x, int16_t y, uint16_t color) {
  drawWarningIconRuns(x, y, color, doorWarningEdgeRuns, DOOR_WARNING_EDGE_RUN_COUNT, doorWarningRuns, DOOR_WARNING_RUN_COUNT);
}

void Graphics::clearTextOverBackground(int16_t x, int16_t y, uint16_t w, uint16_t h, int16_t left_padding, uint8_t right_padding){
	int16_t clearX = x - left_padding;
	int16_t clearY = y;
	int16_t clearW = (int16_t)w + left_padding + (int16_t)right_padding;
	int16_t clearH = (int16_t)h;

	if (clearX < 0) {
		clearW += clearX;
		clearX = 0;
	}
	if (clearY < 0) {
		clearH += clearY;
		clearY = 0;
	}
	if (clearX + clearW > SCREEN_WIDTH) {
		clearW = SCREEN_WIDTH - clearX;
	}
	if (clearY + clearH > SCREEN_HEIGHT) {
		clearH = SCREEN_HEIGHT - clearY;
	}
	if (clearW <= 0 || clearH <= 0) {
		return;
	}

	for (int16_t yScreen = clearY; yScreen < clearY + clearH;) {
		int16_t rowsLeft = clearY + clearH - yScreen;
		uint8_t chunkRows = rowsLeft < BACKGROUND_ROWS_PER_CHUNK ? rowsLeft : BACKGROUND_ROWS_PER_CHUNK;

		for (uint8_t row = 0; row < chunkRows; row++) {
			int16_t yStrip = (yScreen + row) % STRIP_HEIGHT;
			for (int16_t col = 0; col < clearW; col++) {
				backgroundBuffer[row * clearW + col] = pgm_read_word(&(background[yStrip * SCREEN_WIDTH + clearX + col]));
			}
		}

		tft.drawRGBBitmap(clearX, yScreen, backgroundBuffer, clearW, chunkRows);
		yScreen += chunkRows;
	}
}

void Graphics::drawHeaderSeparator() {
  drawLine(0, HEADER_SEPARATOR_Y, tft.width(), HEADER_SEPARATOR_H, ILI9341_RED);
}

void Graphics::drawScreenBackgroundKeepingHeaderSeparator(bool& headerSeparatorDrawn) {
  drawScreenFromStrip(0, HEADER_SEPARATOR_Y);
  if (!headerSeparatorDrawn) {
    drawHeaderSeparator();
    headerSeparatorDrawn = true;
  }
  drawScreenFromStrip(HEADER_SEPARATOR_Y + HEADER_SEPARATOR_H, SCREEN_CONTENT_BOTTOM);
}

void Graphics::drawScreenFromStrip(uint16_t startY, uint16_t endY) {
  if (endY > SCREEN_HEIGHT) {
		endY = SCREEN_HEIGHT;
	}
  if (startY >= endY) {
    return;
  }

  for (uint16_t yScreen = startY; yScreen < endY;) {
    uint16_t rowsLeft = endY - yScreen;
    uint8_t chunkRows = rowsLeft < BACKGROUND_ROWS_PER_CHUNK ? rowsLeft : BACKGROUND_ROWS_PER_CHUNK;

    for (uint8_t row = 0; row < chunkRows; row++) {
      uint16_t yStrip = (yScreen + row) % STRIP_HEIGHT;
      for (uint16_t x = 0; x < SCREEN_WIDTH; x++) {
        backgroundBuffer[row * SCREEN_WIDTH + x] = pgm_read_word(&(background[yStrip * SCREEN_WIDTH + x]));
      }
    }

    tft.drawRGBBitmap(0, yScreen, backgroundBuffer, SCREEN_WIDTH, chunkRows);
    yScreen += chunkRows;
  }
}

void Graphics::drawWarningText(const char* text, int16_t y, uint16_t color, int size) {
  uint16_t w = getTextWidth(text);
  int16_t x = ((tft.width() - w * size) / 2);

	tft.setTextSize(size);
	tft.setCursor(x, y);
	tft.setTextColor(color);
	tft.print(text);
}

void Graphics::drawText(const char* text, int16_t x, int16_t y, TextAlignment aligment, int font, uint8_t max_width, uint16_t color)
{
  switch (font) {
    case 1:
      u8g2.setFont(u8g2_font_cupcakemetoyourleader_tr);
      break;
    case 2:
      u8g2.setFont(u8g2_font_tenstamps_mu);
      break;
    case 3:
      u8g2.setFont(u8g2_font_smart_patrol_nbp_tf);
      break;
    default: 
      u8g2.setFont(u8g2_font_helvR12_tf);
      break;
  }

  uint16_t w = u8g2.getUTF8Width(text);
  int16_t h = u8g2.getFontAscent() - u8g2.getFontDescent();

  switch (aligment) {
    case CENTER:
      x = (SCREEN_WIDTH - w) / 2;
      if (max_width) {
        clearTextOverBackground(x, y - u8g2.getFontAscent(), max_width, h, 0, 0);
      }
      break;
    case RIGHT:
      if (max_width) {
        clearTextOverBackground(x - max_width - 4, y - u8g2.getFontAscent(), max_width + 8, h, 0, 0);
      }
      x = x - w;
      break;
    default:
      if (max_width) {
        clearTextOverBackground(x, y - u8g2.getFontAscent(), max_width, h, 0, 0);
      }
      break;
  }

  u8g2.setForegroundColor(color);
  u8g2.setCursor(x, y);
  u8g2.print(text);
}