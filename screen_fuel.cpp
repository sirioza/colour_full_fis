#include "screen_fuel.h"

void drawFuelScreenLayout(const ScreenLayoutContext& ui, ChangesState& changes, bool& dteDrawn) {
  ui.gfx.drawText("FUEL", 97, 30, ILI9341_WHITE);

  ui.gfx.drawText("Instant", ui.titleIndent, 60 + ui.blockIndent, ILI9341_WHITE);
  ui.gfx.drawText("l", ui.measurementIndent, 60 + ui.blockIndent, ILI9341_WHITE);
  changes.fuelRate = true;

  ui.gfx.drawText("Average", ui.titleIndent, 90 + ui.blockIndent, ILI9341_WHITE);
  ui.gfx.drawText("l/100", ui.measurementIndent, 90 + ui.blockIndent, ILI9341_WHITE);
  changes.fuelAverage = true;

  ui.gfx.drawText("DTE", ui.titleIndent, 120 + ui.blockIndent, ILI9341_WHITE);
  ui.gfx.drawText("km", ui.measurementIndent, 120 + ui.blockIndent, ILI9341_WHITE);
  dteDrawn = false;

  ui.gfx.drawText("Fuel used", ui.titleIndent, 150 + ui.blockIndent, ILI9341_WHITE);
  ui.gfx.drawText("l", ui.measurementIndent, 150 + ui.blockIndent, ILI9341_WHITE);
  changes.fuelUsed = true;

  ui.gfx.drawText("Tank", ui.titleIndent, 180 + ui.blockIndent, ILI9341_WHITE);
  ui.gfx.drawText("l", ui.measurementIndent, 180 + ui.blockIndent, ILI9341_WHITE);
  changes.tank = true;
}
