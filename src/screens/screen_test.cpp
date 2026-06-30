#include <Arduino.h>
#include "screen_test.h"

void drawTestDataScreenLayout(const ScreenLayoutContext& ui, ChangesState& changes) {
  ui.gfx.drawText("Test Data Screen", 0, 30, CENTER);

  ui.gfx.drawText("Impulses", ui.titleIndent, 60 + ui.blockIndent);

  ui.gfx.drawText("Distance", ui.titleIndent, 90 + ui.blockIndent);
  ui.gfx.drawText("m", ui.measurementIndent, 90 + ui.blockIndent);

  ui.gfx.drawText("Idle time", ui.titleIndent, 120 + ui.blockIndent);

  ui.gfx.drawText("GRA", ui.titleIndent, 150 + ui.blockIndent);

  ui.gfx.drawText("Oil press. st.", ui.titleIndent, 180 + ui.blockIndent);

  ui.gfx.drawText("Oil press. dyn.", ui.titleIndent, 210 + ui.blockIndent);

  ui.gfx.drawText("Terminal 30", ui.titleIndent, 240 + ui.blockIndent);
}
