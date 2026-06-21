#pragma once

#include <Arduino.h>
#include "persistent_state.h"
#include "vehicle_utils.h"

extern TripMemoryState tripMemory;

void tripMemoryBegin(uint32_t currentMillis);
bool tripMemoryAwaitingStandTime();
bool resolveTripMemoryByStandTime(Engine5& engine5, Break2& break2, Kombi1& kombi1, const Kombi3& kombi3, uint32_t currentMillis);
bool updateTripMemory(Engine5& engine5, Break2& break2, Kombi1& kombi1, uint32_t currentMillis);
