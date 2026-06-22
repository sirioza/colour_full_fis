#include <Arduino.h>
#include "screen_test.h"

void drawTestDataScreenLayout(const ScreenLayoutContext& ui, ChangesState& changes) {
  ui.gfx.drawText("Test Data Screen", 50, 30, ILI9341_WHITE);

  ui.gfx.drawText("Impulses", ui.titleIndent, 60 + ui.blockIndent, ILI9341_WHITE);

  ui.gfx.drawText("Distance", ui.titleIndent, 90 + ui.blockIndent, ILI9341_WHITE);
  ui.gfx.drawText("m", ui.measurementIndent, 90 + ui.blockIndent, ILI9341_WHITE);
}
