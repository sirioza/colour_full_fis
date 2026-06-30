#include <Arduino.h>
#include "screen_dynamics.h"

void drawDynamicsScreenLayout(const ScreenLayoutContext& ui, ChangesState& changes) {
  ui.gfx.drawText("DYNAMICS", 0, 30, CENTER);

  ui.gfx.drawText("0 - 100 km/h", ui.titleIndent, 60 + ui.blockIndent);
  ui.gfx.drawText("sec", ui.measurementIndent, 60 + ui.blockIndent);
  changes.timeAccel100 = true;

  ui.gfx.drawText("400 m", ui.titleIndent, 90 + ui.blockIndent);
  ui.gfx.drawText("sec", ui.measurementIndent, 90 + ui.blockIndent);
  changes.timeAccel400m = true;
}
