#include "persistent_state.h"
#include <Preferences.h>
#include <math.h>

namespace {
  Preferences prefs;
  bool prefsReady = false;

  const char* NAMESPACE = "g4bc";
  const char* TRIP_KEY = "trip";
  const char* ODOMETER_KEY = "odo";
  const char* DTE_KEY = "dte";
  const uint16_t TRIP_STORE_VERSION = 1;
  const uint16_t ODOMETER_STORE_VERSION = 1;
  const uint16_t DTE_STORE_VERSION = 1;

  const uint32_t TRIP_MAGIC = 0x54524950UL;     // TRIP
  const uint32_t ODOMETER_MAGIC = 0x4F444F4DUL; // ODOM
  const uint32_t DTE_MAGIC = 0x44544531UL;      // DTE1

  struct TripRecord {
    uint32_t magic;
    uint16_t version;
    float averageFuel;
    float fuelUsed;
    uint32_t travelTimeMs;
    float distanceMeters;
    float averageSpeed;
  };

  struct OdometerRecord {
    uint32_t magic;
    uint16_t version;
    uint32_t odometerKm;
  };

  struct DteRecord {
    uint32_t magic;
    uint16_t version;
    float dteKm;
    uint8_t tankLiters;
    float longTermAverageFuel;
  };

  bool cachedTripValid = false;
  bool cachedOdometerValid = false;
  bool cachedDteValid = false;
  TripRecord cachedTrip = {};
  OdometerRecord cachedOdometer = {};
  DteRecord cachedDte = {};

  bool isFiniteNonNegative(float value) {
    return isfinite(value) && value >= 0.0f;
  }

  int32_t quantizeFloat(float value, uint8_t decimals) {
    static const uint32_t factors[] = { 1, 10, 100, 1000 };
    uint8_t index = decimals;
    if (index >= sizeof(factors) / sizeof(factors[0])) {
      index = (sizeof(factors) / sizeof(factors[0])) - 1;
    }

    float factor = (float)factors[index];
    
    return (int32_t)(value >= 0.0f ? value * factor + 0.5f : value * factor - 0.5f);
  }

  uint32_t roundedTravelMinutes(uint32_t travelTimeMs) {
    uint32_t minutes = travelTimeMs / 60000UL;
    return (travelTimeMs % 60000UL) > 30000UL ? minutes + 1 : minutes;
  }

  bool tripRecordChanged(const TripRecord& a, const TripRecord& b) {
    return a.magic != b.magic ||
      a.version != b.version ||
      quantizeFloat(a.averageFuel, 1) != quantizeFloat(b.averageFuel, 1) ||
      quantizeFloat(a.fuelUsed, 2) != quantizeFloat(b.fuelUsed, 2) ||
      roundedTravelMinutes(a.travelTimeMs) != roundedTravelMinutes(b.travelTimeMs) ||
      quantizeFloat(a.distanceMeters, 0) != quantizeFloat(b.distanceMeters, 0) ||
      quantizeFloat(a.averageSpeed, 0) != quantizeFloat(b.averageSpeed, 0);
  }

  bool odometerRecordChanged(const OdometerRecord& a, const OdometerRecord& b) {
    return a.magic != b.magic ||
      a.version != b.version ||
      a.odometerKm != b.odometerKm;
  }

  bool dteRecordChanged(const DteRecord& a, const DteRecord& b) {
    return a.magic != b.magic ||
      a.version != b.version ||
      quantizeFloat(a.dteKm, 0) != quantizeFloat(b.dteKm, 0) ||
      a.tankLiters != b.tankLiters ||
      quantizeFloat(a.longTermAverageFuel, 1) != quantizeFloat(b.longTermAverageFuel, 1);
  }
}

void persistentStateBegin() {
  prefsReady = prefs.begin(NAMESPACE, false);
  if (!prefsReady) {
    Serial.println("[NVS] persistent state begin failed");
  }
}

bool loadTripMemory(TripMemoryState& state) {
  TripRecord record = {};
  state = {};

  if (!prefsReady) {
    return false;
  }

  if (prefs.getBytes(TRIP_KEY, &record, sizeof(record)) != sizeof(record)) {
    return false;
  }

  if (record.magic != TRIP_MAGIC || record.version != TRIP_STORE_VERSION ||
      !isFiniteNonNegative(record.averageFuel) ||
      !isFiniteNonNegative(record.fuelUsed) ||
      !isFiniteNonNegative(record.distanceMeters) ||
      !isFiniteNonNegative(record.averageSpeed)) {
    return false;
  }

  state.valid = true;
  state.averageFuel = record.averageFuel;
  state.fuelUsed = record.fuelUsed;
  state.travelTimeMs = record.travelTimeMs;
  state.distanceMeters = record.distanceMeters;
  state.averageSpeed = record.averageSpeed;
  cachedTrip = record;
  cachedTripValid = true;
  
  return true;
}

bool loadOdometerMemory(OdometerMemoryState& state) {
  OdometerRecord record = {};
  state = {};

  if (!prefsReady) {
    return false;
  }

  if (prefs.getBytes(ODOMETER_KEY, &record, sizeof(record)) != sizeof(record)) {
    return false;
  }

  if (record.magic != ODOMETER_MAGIC || record.version != ODOMETER_STORE_VERSION) {
    return false;
  }

  state.valid = true;
  state.odometerKm = record.odometerKm;
  cachedOdometer = record;
  cachedOdometerValid = true;
  return true;
}

bool loadDteMemory(DteMemoryState& state) {
  DteRecord record = {};
  state = {};

  if (!prefsReady) {
    return false;
  }

  if (prefs.getBytes(DTE_KEY, &record, sizeof(record)) != sizeof(record)) {
    return false;
  }

  if (record.magic != DTE_MAGIC || record.version != DTE_STORE_VERSION ||
      !isFiniteNonNegative(record.dteKm) ||
      !isFiniteNonNegative(record.longTermAverageFuel)) {
    return false;
  }

  state.valid = true;
  state.dteKm = record.dteKm;
  state.tankLiters = record.tankLiters;
  state.longTermAverageFuel = record.longTermAverageFuel;
  cachedDte = record;
  cachedDteValid = true;
  return true;
}

void saveTripMemory(const TripMemoryState& state) {
  if (!prefsReady || !state.valid) {
    return;
  }

  TripRecord record = {
    TRIP_MAGIC,
    TRIP_STORE_VERSION,
    state.averageFuel,
    state.fuelUsed,
    state.travelTimeMs,
    state.distanceMeters,
    state.averageSpeed
  };

  if (cachedTripValid && !tripRecordChanged(record, cachedTrip)) {
    return;
  }

  if (prefs.putBytes(TRIP_KEY, &record, sizeof(record)) != sizeof(record)) {
    return;
  }
  cachedTrip = record;
  cachedTripValid = true;
}

void saveOdometerMemory(const OdometerMemoryState& state) {
  if (!prefsReady || !state.valid) {
    return;
  }

  OdometerRecord record = {
    ODOMETER_MAGIC,
    ODOMETER_STORE_VERSION,
    state.odometerKm
  };

  if (cachedOdometerValid && !odometerRecordChanged(record, cachedOdometer)) {
    return;
  }

  if (prefs.putBytes(ODOMETER_KEY, &record, sizeof(record)) != sizeof(record)) {
    return;
  }
  cachedOdometer = record;
  cachedOdometerValid = true;
}

void saveDteMemory(const DteMemoryState& state) {
  if (!prefsReady || !state.valid) {
    return;
  }

  DteRecord record = {
    DTE_MAGIC,
    DTE_STORE_VERSION,
    state.dteKm,
    state.tankLiters,
    state.longTermAverageFuel
  };

  if (cachedDteValid && !dteRecordChanged(record, cachedDte)) {
    return;
  }

  if (prefs.putBytes(DTE_KEY, &record, sizeof(record)) != sizeof(record)) {
    return;
  }
  cachedDte = record;
  cachedDteValid = true;
}
