#include <Arduino.h>
#include "screen_speed.h"

void drawSpeedScreenLayout(const ScreenLayoutContext& ui, ChangesState& changes, bool& travelTimeDrawn) {
  ui.gfx.drawText("SPEED", 0, 30, CENTER);

  /*ui.gfx.drawText("Speed", ui.titleIndent, 60 + ui.blockIndent);
  ui.gfx.drawText("km/h", ui.measurementIndent, 60 + ui.blockIndent);
  changes.speed = true;

  ui.gfx.drawText("Aver. speed", ui.titleIndent, 90 + ui.blockIndent);
  ui.gfx.drawText("km/h", ui.measurementIndent, 90 + ui.blockIndent);
  changes.avgSpeed = true;

  ui.gfx.drawText("Cover. distance", ui.titleIndent, 120 + ui.blockIndent);
  ui.gfx.drawText("km", ui.measurementIndent, 120 + ui.blockIndent);
  changes.distance = true;

  ui.gfx.drawText("Travel time", ui.titleIndent, 150 + ui.blockIndent);
  travelTimeDrawn = false;

  ui.gfx.drawText("Odometer", ui.titleIndent, 180 + ui.blockIndent);
  ui.gfx.drawText("km", ui.measurementIndent, 180 + ui.blockIndent);
  changes.odometer = true;*/
}
