#include <Arduino.h>
#include "vehicle_utils.h"
#include <stdint.h>
#include "string_utils.h"

ChangesState changes_status;
ErrorState error_state;

const int16_t  IMPULSE_MAX_VALUE = 4095;
const int16_t  IMPULSE_MIN_VALUE = 2048;
const float  IMPULSES_KOEF = 45.0f;

const float MIN_KM = 0.03f;
const float SMOOTH_KM = 2.0f;
const float DEFAULT_FUEL = 15.5f;

const uint8_t IMPLAUSIBLE_VALUE_HEX = 0xFF;
const float DEFAULT_ENGINE_TORQUE_NM = 170.0f;
const float KW_TO_HP_FACTOR = 1.35962f;
const float TORQUE_PERCENT_FACTOR = 0.39f;
const float TORQUE_NORM_FACTOR = 10.0f;
const float DOUBLE_TORQUE_NORM_FACTOR = 20.0f;

static float engineTorqueNormNm = DEFAULT_ENGINE_TORQUE_NM;

void parseEngine1(uint8_t buf[8], Engine1 &s) {
  uint16_t rpmOld = s.rpm;
  s.rpm = (((uint16_t)buf[3] << 8) | buf[2]) >> 2;
  changes_status.rpm = diffInt16(s.rpm, rpmOld);

  s.isRunning = s.rpm != 0;

  uint8_t rawActualTorque = buf[1];
  if (rawActualTorque == IMPLAUSIBLE_VALUE_HEX) {
    return;
  }

  uint8_t rawLossTorque = buf[6];
  uint8_t rawDriverRequestTorque = buf[7];
  s.torquePercent = (float)rawActualTorque * TORQUE_PERCENT_FACTOR;
  s.indicatedTorque = engineTorqueNormNm * (s.torquePercent / 100.0f);
  s.lossTorque = rawLossTorque == IMPLAUSIBLE_VALUE_HEX
    ? 0.0f
    : engineTorqueNormNm * (((float)rawLossTorque * TORQUE_PERCENT_FACTOR) / 100.0f);
  s.driverRequestTorque = rawDriverRequestTorque == IMPLAUSIBLE_VALUE_HEX
    ? 0.0f
    : engineTorqueNormNm * (((float)rawDriverRequestTorque * TORQUE_PERCENT_FACTOR) / 100.0f);

  float torqueOld = s.torque;
  s.torque = s.indicatedTorque - s.lossTorque;
  if (s.torque < 0.0f) {
    s.torque = 0.0f;
  }
  changes_status.torque = diffFloat(s.torque, torqueOld, 1);

  float powerHpOld = s.powerHp;
  s.powerHp = s.torque * s.rpm * (KW_TO_HP_FACTOR / 9549.0f);
  changes_status.powerHp = diffFloat(s.powerHp, powerHpOld, 1);
}

void parseEngine2(uint8_t buf[8], Engine2 &s) {
  uint8_t muxCode = (buf[0] >> 6) & 0x03;
  uint8_t muxInfo = buf[0] & 0x3F;
  if (muxCode == 0x03 && muxInfo != 0) {
    s.torqueNormNm = (float)muxInfo * TORQUE_NORM_FACTOR;
  }

  s.coolantError = (buf[2] & 0x04) != 0;

  int16_t coolantTempOld = s.coolantTemp;
  s.coolantTemp = (int16_t)(((uint16_t)buf[1] * 3 + 2) / 4) - 48;
  changes_status.coolantTemp = diffInt16(s.coolantTemp, coolantTempOld);

  int8_t graStatusOld = s.graStatus;
  s.graStatus = (buf[2] >> 6) & 0x03;
  changes_status.graStatus = diffInt8(s.graStatus, graStatusOld);
}

void parseEngine5(uint8_t buf[8], Engine5 &s, uint32_t time_now, float distance) {
  s.fuelAverageUpdated = false;

  uint8_t muxCode = (buf[0] >> 6) & 0x03;
  uint8_t muxInfo = buf[0] & 0x3F;
  if (muxCode == 0x00 && muxInfo != 0) {
    float maxTorqueNmOld = s.maxTorqueNm;
    s.doubleTorqueScale = (buf[6] & 0x08) != 0;
    s.maxTorqueNm = (float)muxInfo * (s.doubleTorqueScale ? DOUBLE_TORQUE_NORM_FACTOR : TORQUE_NORM_FACTOR);
    engineTorqueNormNm = s.maxTorqueNm;
    changes_status.torque = changes_status.torque || diffFloat(s.maxTorqueNm, maxTorqueNmOld, 1);
  }

  s.fuelNow = ((uint16_t)(buf[3] & 0x7F) << 8) | (uint16_t)buf[2];

  if (!s.inited) {
    s.distZero = s.useAbsoluteDistanceForAverage ? 0.0f : distance;
    s.distLast = distance;
    s.fuelLast = s.fuelNow;
    s.inited = true;
    return;
  }

  float fuel_used_for_cycle = (s.fuelNow >= s.fuelLast)
    ? (s.fuelNow - s.fuelLast)
    : (s.fuelNow + 32768 - s.fuelLast);

  float fuelUsedOld = s.fuelUsed;
  s.fuelUsed += fuel_used_for_cycle / 1000000.0f;
  changes_status.fuelUsed = changes_status.fuelUsed || diffFloat(s.fuelUsed, fuelUsedOld, 2);

  float dist_delta = distance - s.distLast;
  if (dist_delta < 0.0f){
    dist_delta = 0.0f;
  }

  float total_km = (distance - s.distZero) / 1000.0f;
  if (total_km >= MIN_KM && s.fuelUsed > 0.0f) {
    float effective_km = total_km;
    if (dist_delta < 1.0f) {
      effective_km += 0.05f;
    }

    float real_avg = s.fuelUsed / (effective_km / 100.0f);
    float avg;

    if (total_km < SMOOTH_KM) {
      float t = total_km / SMOOTH_KM;
      avg = DEFAULT_FUEL + (real_avg - DEFAULT_FUEL) * t;
    } else {
      avg = real_avg;
    }

    if (avg != s.fuelAverage) {
      float old = s.fuelAverage;
      s.fuelAverage = avg;
      bool changed = diffFloat(s.fuelAverage, old, 1);
      changes_status.fuelAverage = changes_status.fuelAverage || changed;
      s.fuelAverageUpdated = changed;
    }
  } else if (distance != 0 && total_km <= MIN_KM) {
    float old = s.fuelAverage;
    s.fuelAverage = DEFAULT_FUEL;
    bool changed = diffFloat(s.fuelAverage, old, 1);
    changes_status.fuelAverage = changes_status.fuelAverage || changed;
    s.fuelAverageUpdated = changed;
  }

  uint32_t dt = time_now - s.timeLast;
  if (s.timeLast != 0 && dt > 0) {
    float fuelRateRaw = (float)fuel_used_for_cycle * 3.6f / (float)dt;

    s.fuelBuffer[s.fuelPointer] = fuelRateRaw;
    s.fuelPointer++;

    if (s.fuelPointer >= 10) {
      s.fuelPointer = 0;
    }

    float fuelRateFiltered = 0.0f;
    for (uint8_t i = 0; i < 10; i++) {
      fuelRateFiltered += s.fuelBuffer[i];
    }
    
    fuelRateFiltered /= 10.0f;

    float fuelRateOld = s.fuelRate;
    s.fuelRate = fuelRateFiltered;

    changes_status.fuelRate = diffFloat(s.fuelRate, fuelRateOld, 1);
  }

  s.fuelLast = s.fuelNow;
  s.timeLast = time_now;
  s.distLast = distance;
}

void resetEngine5Trip(Engine5 &s) {
  s.fuelAverage = 0.0f;
  s.fuelUsed = 0.0f;
  s.useAbsoluteDistanceForAverage = false;
  s.distZero = 0.0f;
  s.distLast = 0.0f;
}

void parseBreak1(uint8_t buf[8], Break1 &s) {
  s.absLamp = (buf[1] & 0x01) != 0;
  s.absMessageTimeout = false;
  s.absErr = s.absLamp || s.absMessageTimeout;
  updateAbsWarningState(s);

  s.brakeWarningLamp = (buf[1] & 0x04) != 0;
  s.brakePedalActive = (buf[1] & 0x08) != 0;
  s.breakErr = s.brakeWarningLamp;
  s.absDiag = ((buf[1] >> 7) & 0x01) != 0;
}

void updateAbsWarningState(Break1 &s) {
  if (s.absErr) {
    if (!s.showAbsFullscreenWarn && !s.absFullscreenWarnCollapsed) {
      s.absWarnActiveTime = 0;
      s.showAbsFullscreenWarn = true;
    }
  } else {
    s.absWarnActiveTime = 0;
    s.showAbsFullscreenWarn = false;
    s.absFullscreenWarnCollapsed = false;
  }
}

void parseBreak2(uint8_t buf[8], Break2 &s, uint32_t time_now, bool measurement_mode) {
  uint16_t currentImpulses = ((uint16_t)buf[6] << 8) | buf[5];

  if (currentImpulses == 0) {
    s.impulses = 0;
    return;
  }

  if (!s.initialized) {
    s.impulses = currentImpulses;
    s.initialized = true;
    return;
  }

  int16_t delta;
  if (currentImpulses >= s.impulses) {
    // first 0→4095 and next 2048→4095
    delta = currentImpulses - s.impulses;
  } else {
    // 4095→2048
    delta = (IMPULSE_MAX_VALUE - s.impulses + 1) + (currentImpulses - IMPULSE_MIN_VALUE);
  }

  int32_t impulsesOld = s.totalImpulses;
  s.totalImpulses += delta;
  changes_status.impulses = changes_status.impulses || diffInt32(s.totalImpulses, impulsesOld);

  float distanceBefore = s.totalDistance;
  float currentTotalDistance = (float)delta / IMPULSES_KOEF;

  s.totalDistance += currentTotalDistance;
  s.impulses = currentImpulses;
  changes_status.distance = changes_status.distance || diffFloat(currentTotalDistance, 0.0f, 1);

  if (!measurement_mode){
    s.timeStart = 0;
    s.startDistance = 0;
    s.reached400m = false;
    s.timeAccel400m = 0;
    s.timeLast = 0;
    return;
  }

  if (!s.reached400m) {
    if (s.timeStart == 0 && currentTotalDistance > 0.0f) {
      s.timeStart = time_now;
      s.startDistance = distanceBefore;
    }

    if (s.totalDistance - s.startDistance >= 400.0f && s.timeStart > 0) {
      float overDistance = (s.totalDistance - s.startDistance) - 400.0f;
      float frameDistance = s.totalDistance - distanceBefore;
      uint32_t frameTime = s.timeLast == 0 ? 0 : time_now - s.timeLast;
      float correction = frameDistance > 0.0f ? (overDistance / frameDistance) * ((float)frameTime / 1000.0f) : 0.0f;
      s.timeAccel400m = ((time_now - s.timeStart) / 1000.0f) - correction;
      s.reached400m = true;
      changes_status.timeAccel400m = true;
    }
  }

  s.timeLast = time_now;
}

void resetBreak2Trip(Break2 &s) {
  s.totalDistance = 0.0f;
}

void parseKombi1(uint8_t buf[8], Kombi1 &s, uint32_t time_now, bool measurement_mode) {
  s.tankWarn = (buf[2] & 0x80) != 0;
  s.tankEmpty = s.tankWarn;
  s.tankError = (buf[0] & 0x02) != 0;
  if (s.tankEmpty) {
    if (!s.showTankFullscreenWarn && !s.tankFullscreenWarnCollapsed) {
      s.tankWarnActiveTime = 0;
      s.showTankFullscreenWarn = true;
    }
  } else {
    s.tankWarnActiveTime = 0;
    s.showTankFullscreenWarn = false;
    s.tankFullscreenWarnCollapsed = false;
  }

  s.oilPressureSt = ((buf[0] >> 2) & 0x01) != 0;
  s.oilPressureDyn = ((buf[0] >> 3) & 0x01) != 0;
  s.coolantLow = ((buf[0] >> 4) & 0x01) != 0;
  s.coolantHot = ((buf[0] >> 5) & 0x01) != 0;

  uint8_t tankOld = s.tank;
  s.tank = buf[2] & 0x7F;
  changes_status.tank = diffUint8(s.tank, tankOld);

  s.driverDoorOpen = (buf[0] & 0x01) != 0;
  s.handBrakeActive = (buf[1] & 0x03) == 0x02;

  //SPEED
  float speedOld = s.speed;
  uint16_t raw_speed_value = (buf[4] << 7) | ((buf[3] >> 1) & 0x7F);
  s.speed = (float)raw_speed_value * 0.01f;
  changes_status.speed = diffFloat(s.speed, speedOld, 1);

  // count avg speed
  if (s.timeLast != 0) {
    float dt = (time_now - s.timeLast) / 1000.0f; 		// Δt sec
    s.sumSpeedTime += s.speed * dt;
    s.sumTime += dt;
  }

  s.timeLast = time_now;

  float avgSpeedOld = s.avgSpeed;
  if (s.sumTime > 0) {
    s.avgSpeed = s.sumSpeedTime / s.sumTime;
  } else {
    s.avgSpeed = 0.0f;
  }
  changes_status.avgSpeed = changes_status.avgSpeed || diffInt16(s.avgSpeed, avgSpeedOld);

  if (!measurement_mode){
    return;
  }

  float timeAccel100Old = s.timeAccel100;
  if (!s.reachedSpeed100) {
    if (s.speed < 1.0) {
      s.accelStartTime = time_now;
    }

    if (s.speed >= 100.0 && s.accelStartTime > 0) {
      s.timeAccel100 = (time_now - s.accelStartTime) / 1000.0;
      s.reachedSpeed100 = true;
    }
  }

  if (s.speed < 1.0f && s.reachedSpeed100) {
    s.reachedSpeed100 = false;
    s.timeAccel100 = 0;
    s.accelStartTime = 0;
  }

  changes_status.timeAccel100 = diffFloat(s.timeAccel100, timeAccel100Old, 1);
}

void resetKombi1Trip(Kombi1 &s) {
  s.avgSpeed = 0.0f;
  s.sumSpeedTime = 0.0f;
  s.sumTime = 0.0f;
}

void parseKombi2(uint8_t buf[8], Kombi2 &s) {
  uint8_t raw = buf[1];
  s.validOutsideTemp = (buf[0] & 0x01) == 0 && raw != 0xFF && raw >= 5;
  if (!s.validOutsideTemp) {
    return;
  }

  float outsideTempOld = s.outsideTemp;
  s.outsideTemp = raw * 0.5f - 50.0f;
  changes_status.outsideTemp = diffFloat(s.outsideTemp, outsideTempOld, 1);
}

void parseKombi3(uint8_t buf[8], Kombi3 &s) {
  uint32_t odometerOld = s.odometer;
  s.odometer = (uint32_t)buf[5] | ((uint32_t)buf[6] << 8) | ((uint32_t)(buf[7] & 0x0F) << 16);
  changes_status.odometer = diffInt32(s.odometer, odometerOld);

  s.standTimeError = (buf[4] & 0x80) != 0;
  s.standTime = (((uint32_t)(buf[4] & 0x7F) << 8) | buf[3]) * 4;

  uint32_t idleTimeOld = s.odometer;
  s.idleTime = s.standTime;
  changes_status.idleTime = diffInt32(s.idleTime, idleTimeOld);
}

void parseAirbag1(uint8_t buf[8], Airbag1 &s, uint32_t time_now) {
  if (time_now >= 7000) {
    s.airbagLamp = (buf[1] & 0x01) != 0;
    s.airbagErr = s.airbagLamp;
  }
  
  if (s.airbagErr) {
    if (!s.showAirbagFullscreenWarn && !s.airbagFullscreenWarnCollapsed) {
      s.airWarnActiveTime = 0;
      s.showAirbagFullscreenWarn = true;
    }
  } else {
    s.airWarnActiveTime = 0;
    s.showAirbagFullscreenWarn = false;
    s.airbagFullscreenWarnCollapsed = false;
  }
}

bool diffUint8(uint8_t a, uint8_t b) {
  return a != b;
}

bool diffInt8(int8_t a, int8_t b) {
  return a != b;
}

bool diffInt16(uint16_t a, uint16_t b) {
  return a != b;
}

bool diffInt32(uint32_t a, uint32_t b) {
  return a != b;
}

bool diffFloat(float a, float b, uint8_t decimals) {
  static const uint32_t factors[] = { 1, 10, 100, 1000, 10000, 100000 };
  uint8_t index = decimals;
  if (index >= sizeof(factors) / sizeof(factors[0])) {
    index = (sizeof(factors) / sizeof(factors[0])) - 1;
  }
  uint32_t factor = factors[index];
  int32_t ai = (int32_t)(a >= 0.0f ? a * (float)factor + 0.5f : a * (float)factor - 0.5f);
  int32_t bi = (int32_t)(b >= 0.0f ? b * (float)factor + 0.5f : b * (float)factor - 0.5f);
  return ai != bi;
}
