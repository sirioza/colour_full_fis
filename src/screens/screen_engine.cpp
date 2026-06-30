#include <Arduino.h>
#include "screen_engine.h"

void drawEngineScreenLayout(const ScreenLayoutContext& ui, ChangesState& changes) {
  ui.gfx.drawText("ENGINE", 0, 30, CENTER);

  ui.gfx.drawText("RPM", ui.titleIndent, 60 + ui.blockIndent);
  changes.rpm = true;

  ui.gfx.drawText("Torque", ui.titleIndent, 90 + ui.blockIndent);
  ui.gfx.drawText("Nm", ui.measurementIndent, 90 + ui.blockIndent);
  changes.torque = true;

  ui.gfx.drawText("Power", ui.titleIndent, 120 + ui.blockIndent);
  changes.powerHp = true;
  ui.gfx.drawText("HP", ui.measurementIndent, 120 + ui.blockIndent);

  ui.gfx.drawText("Coolant", ui.titleIndent, 150 + ui.blockIndent);
  ui.tft.drawCircle(ui.measurementIndent + 3, 150 + ui.blockIndent - 10, 2, ILI9341_WHITE);
  ui.gfx.drawText("C", ui.measurementIndent + 7, 150 + ui.blockIndent);
  changes.coolantTemp = true;

  ui.gfx.drawText("Outside temp.", ui.titleIndent, 180 + ui.blockIndent);
  ui.tft.drawCircle(ui.measurementIndent + 3, 180 + ui.blockIndent - 10, 2, ILI9341_WHITE);
  ui.gfx.drawText("C", ui.measurementIndent + 7, 180 + ui.blockIndent);
  changes.outsideTemp = true;
}
