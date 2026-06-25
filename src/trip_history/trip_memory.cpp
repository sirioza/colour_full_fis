#include "trip_memory.h"

namespace {
  const uint32_t TRIP_MEMORY_TIMEOUT_MS = 2UL * 60UL * 60UL * 1000UL;
  const uint32_t TRIP_MEMORY_TIMEOUT_SEC = TRIP_MEMORY_TIMEOUT_MS / 1000UL;
  const uint32_t TRIP_STOP_SAVE_INTERVAL_MS = 30000UL;
  const float TRIP_SAVE_SPEED_KMH = 2.0f;
  const float STOP_SPEED_KMH = 0.5f;

  TripMemoryState pendingTripMemory;
  uint32_t tripBaseTravelTimeMs = 0;
  uint32_t tripSessionStartMs = 0;
  uint32_t tripTimeoutStartMs = 0;
  uint32_t lastTripStoppedSaveMs = 0;
  bool pendingTripMemoryValid = false;
  bool tripMemoryAwaitingKombi3 = true;
  bool tripStopSaved = false;
  bool tripTimeRunning = false;

  void resetTripAccumulators(Engine5& engine5, Break2& break2, Kombi1& kombi1) {
    resetEngine5Trip(engine5);
    resetBreak2Trip(break2);
    resetKombi1Trip(kombi1);
  }

  bool applyTripMemory(Engine5& engine5, Break2& break2, Kombi1& kombi1, uint32_t currentMillis) {
    tripMemory = pendingTripMemory;
    tripMemory.valid = true;

    tripBaseTravelTimeMs = tripMemory.travelTimeMs;
    tripSessionStartMs = currentMillis;
    tripTimeoutStartMs = currentMillis;
    tripTimeRunning = false;

    engine5.fuelAverage = tripMemory.averageFuel;
    engine5.fuelUsed = tripMemory.fuelUsed;
    engine5.useAbsoluteDistanceForAverage = true;
    engine5.distZero = 0.0f;
    engine5.distLast = tripMemory.distanceMeters;
    break2.totalDistance = tripMemory.distanceMeters;
    kombi1.avgSpeed = tripMemory.averageSpeed;
    if (tripMemory.travelTimeMs > 0) {
      kombi1.sumTime = (float)tripMemory.travelTimeMs / 1000.0f;
      kombi1.sumSpeedTime = kombi1.avgSpeed * kombi1.sumTime;
    } else {
      kombi1.sumTime = 0.0f;
      kombi1.sumSpeedTime = 0.0f;
    }

    pendingTripMemory = {};
    pendingTripMemoryValid = false;
    tripMemoryAwaitingKombi3 = false;
    return true;
  }

  bool resetTripMemory(Engine5& engine5, Break2& break2, Kombi1& kombi1, uint32_t currentMillis, bool persist) {
    tripMemory.valid = true;
    tripMemory.averageFuel = 0.0f;
    tripMemory.fuelUsed = 0.0f;
    tripMemory.travelTimeMs = 0;
    tripMemory.distanceMeters = 0.0f;
    tripMemory.averageSpeed = 0.0f;
    tripBaseTravelTimeMs = 0;
    tripSessionStartMs = currentMillis;
    tripTimeoutStartMs = currentMillis;
    tripTimeRunning = false;
    pendingTripMemory = {};
    pendingTripMemoryValid = false;
    tripMemoryAwaitingKombi3 = false;
    tripStopSaved = false;
    lastTripStoppedSaveMs = 0;

    resetTripAccumulators(engine5, break2, kombi1);
    if (persist) {
      saveTripMemory(tripMemory);
    }
    return true;
  }
}

TripMemoryState tripMemory;

void tripMemoryBegin(uint32_t currentMillis) {
  tripMemory = {};
  tripMemory.valid = true;
  pendingTripMemory = {};
  pendingTripMemoryValid = loadTripMemory(pendingTripMemory);
  tripMemoryAwaitingKombi3 = true;
  tripBaseTravelTimeMs = tripMemory.travelTimeMs;
  tripSessionStartMs = currentMillis;
  tripTimeoutStartMs = currentMillis;
  tripTimeRunning = false;
  tripStopSaved = false;
  lastTripStoppedSaveMs = 0;
}

bool tripMemoryAwaitingStandTime() {
  return tripMemoryAwaitingKombi3;
}

bool resolveTripMemoryByStandTime(Engine5& engine5, Break2& break2, Kombi1& kombi1, const Kombi3& kombi3, uint32_t currentMillis) {
  if (!tripMemoryAwaitingKombi3) {
    return false;
  }

  if (pendingTripMemoryValid && !kombi3.standTimeError && kombi3.standTime <= TRIP_MEMORY_TIMEOUT_SEC) {
    return applyTripMemory(engine5, break2, kombi1, currentMillis);
  }
  return resetTripMemory(engine5, break2, kombi1, currentMillis, false);
}

bool updateTripMemory(Engine5& engine5, Break2& break2, Kombi1& kombi1, uint32_t currentMillis) {
  // Live trip accumulators may run before KOMBI3; only persisted restore/save waits for KOMBI3.
  if (tripMemoryAwaitingKombi3) {
    return false;
  }

  bool moving = kombi1.speed > STOP_SPEED_KMH;
  bool consumingFuel = engine5.fuelRate > 0.01f;
  bool tripActiveForTimeout = moving || consumingFuel;
  bool stoppedForPersistence = kombi1.speed < TRIP_SAVE_SPEED_KMH;

  if (tripActiveForTimeout) {
    if (!tripTimeRunning) {
      tripSessionStartMs = currentMillis;
      tripTimeRunning = true;
    }
    tripTimeoutStartMs = currentMillis;
    tripMemory.travelTimeMs = tripBaseTravelTimeMs + (currentMillis - tripSessionStartMs);
  } else {
    if (tripTimeRunning) {
      tripBaseTravelTimeMs += currentMillis - tripSessionStartMs;
      tripTimeRunning = false;
    }
    tripMemory.travelTimeMs = tripBaseTravelTimeMs;
  }

  tripMemory.valid = true;
  tripMemory.averageFuel = engine5.fuelAverage;
  tripMemory.fuelUsed = engine5.fuelUsed;
  tripMemory.distanceMeters = break2.totalDistance;
  tripMemory.averageSpeed = kombi1.avgSpeed;

  if (!stoppedForPersistence) {
    tripStopSaved = false;
    lastTripStoppedSaveMs = 0;
    return false;
  }

  bool shouldSaveStoppedTrip = !tripStopSaved ||
    lastTripStoppedSaveMs == 0 ||
    (uint32_t)(currentMillis - lastTripStoppedSaveMs) >= TRIP_STOP_SAVE_INTERVAL_MS;
  if (shouldSaveStoppedTrip) {
    saveTripMemory(tripMemory);
    tripStopSaved = true;
    lastTripStoppedSaveMs = currentMillis;
  }

  if ((uint32_t)(currentMillis - tripTimeoutStartMs) >= TRIP_MEMORY_TIMEOUT_MS) {
    return resetTripMemory(engine5, break2, kombi1, currentMillis, true);
  }
  return false;
}
