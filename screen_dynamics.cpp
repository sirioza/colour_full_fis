#include "screen_dynamics.h"

void drawDynamicsScreenLayout(const ScreenLayoutContext& ui, ChangesState& changes) {
  ui.gfx.drawText("DYNAMICS", 73, 30, ILI9341_WHITE);

  ui.gfx.drawText("0 - 100 km/h", ui.titleIndent, 60 + ui.blockIndent, ILI9341_WHITE);
  ui.gfx.drawText("sec", ui.measurementIndent, 60 + ui.blockIndent, ILI9341_WHITE);
  changes.timeAccel100 = true;

  ui.gfx.drawText("400 m", ui.titleIndent, 90 + ui.blockIndent, ILI9341_WHITE);
  ui.gfx.drawText("sec", ui.measurementIndent, 90 + ui.blockIndent, ILI9341_WHITE);
  changes.timeAccel400m = true;
}
