#include <Arduino.h>
#include "screen_warnings.h"

void drawWarningsScreenLayout(const ScreenLayoutContext& ui) {
  ui.gfx.drawText("WARNINGS", 0, 30, CENTER);
}
