#include <Arduino.h>
#include "screen_speed.h"

void drawSpeedScreenLayout(const ScreenLayoutContext& ui, ChangesState& changes, bool& travelTimeDrawn) {
  ui.gfx.drawText("SPEED", 91, 30, ILI9341_WHITE);

  ui.gfx.drawText("Speed", ui.titleIndent, 60 + ui.blockIndent, ILI9341_WHITE);
  ui.gfx.drawText("km/h", ui.measurementIndent, 60 + ui.blockIndent, ILI9341_WHITE);
  changes.speed = true;

  ui.gfx.drawText("Aver. speed", ui.titleIndent, 90 + ui.blockIndent, ILI9341_WHITE);
  ui.gfx.drawText("km/h", ui.measurementIndent, 90 + ui.blockIndent, ILI9341_WHITE);
  changes.avgSpeed = true;

  ui.gfx.drawText("Cover. distance", ui.titleIndent, 120 + ui.blockIndent, ILI9341_WHITE);
  ui.gfx.drawText("km", ui.measurementIndent, 120 + ui.blockIndent, ILI9341_WHITE);
  changes.distance = true;

  ui.gfx.drawText("Travel time", ui.titleIndent, 150 + ui.blockIndent, ILI9341_WHITE);
  travelTimeDrawn = false;

  ui.gfx.drawText("Odometer", ui.titleIndent, 180 + ui.blockIndent, ILI9341_WHITE);
  ui.gfx.drawText("km", ui.measurementIndent, 180 + ui.blockIndent, ILI9341_WHITE);
  changes.odometer = true;
}
