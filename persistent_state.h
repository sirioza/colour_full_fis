#pragma once
#include <Arduino.h>

struct TripMemoryState {
  bool valid;
  float averageFuel;
  float fuelUsed;
  uint32_t travelTimeMs;
  float distanceMeters;
  float averageSpeed;
};

struct OdometerMemoryState {
  bool valid;
  uint32_t odometerKm;
};

struct DteMemoryState {
  bool valid;
  float dteKm;
  uint8_t tankLiters;
  float longTermAverageFuel;
};

void persistentStateBegin();
bool loadTripMemory(TripMemoryState& state);
bool loadOdometerMemory(OdometerMemoryState& state);
bool loadDteMemory(DteMemoryState& state);
void saveTripMemory(const TripMemoryState& state);
void saveOdometerMemory(const OdometerMemoryState& state);
void saveDteMemory(const DteMemoryState& state);
