#pragma once

#include <Arduino.h>
#include "screen_context.h"
#include "../utils/vehicle_utils.h"

void drawSpeedScreenLayout(const ScreenLayoutContext& ui, ChangesState& changes, bool& travelTimeDrawn);
