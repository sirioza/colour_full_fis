#include <Arduino.h>
#include "screen_fuel.h"

void drawFuelScreenLayout(const ScreenLayoutContext& ui, ChangesState& changes, bool& dteDrawn) {
  ui.gfx.drawText("FUEL", 0, 30, CENTER);

  ui.gfx.drawText("Instant", ui.titleIndent, 60 + ui.blockIndent);
  ui.gfx.drawText("l", ui.measurementIndent, 60 + ui.blockIndent);
  changes.fuelRate = true;

  ui.gfx.drawText("Average", ui.titleIndent, 90 + ui.blockIndent);
  ui.gfx.drawText("l/100", ui.measurementIndent, 90 + ui.blockIndent);
  changes.fuelAverage = true;

  ui.gfx.drawText("DTE", ui.titleIndent, 120 + ui.blockIndent);
  ui.gfx.drawText("km", ui.measurementIndent, 120 + ui.blockIndent);
  dteDrawn = false;

  ui.gfx.drawText("Fuel used", ui.titleIndent, 150 + ui.blockIndent);
  ui.gfx.drawText("l", ui.measurementIndent, 150 + ui.blockIndent);
  changes.fuelUsed = true;

  ui.gfx.drawText("Tank", ui.titleIndent, 180 + ui.blockIndent);
  ui.gfx.drawText("l", ui.measurementIndent, 180 + ui.blockIndent);
  changes.tank = true;
}
