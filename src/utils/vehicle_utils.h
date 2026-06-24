#pragma once

#include <Arduino.h>

struct Engine1 {
    uint16_t rpm;
    float torque;
    float torquePercent;
    float indicatedTorque;
    float lossTorque;
    float driverRequestTorque;
    bool isRunning;
    float powerHp;
};

struct Engine2 {
    bool coolantError;                  // true = датчик температуры ОЖ невалиден, ECU работает с заглушкой.
    int16_t coolantTemp;
    float torqueNormNm;
};

struct Engine5 {
    bool inited;
    uint16_t fuelNow;
    uint16_t fuelLast;
    float fuelAverage;      			// l/100 - fuel average consumption
    float fuelBuffer[50] = {0};
    uint8_t fuelPointer = 0;
    float fuelRate;                 	// l - fuel instantan consumption
    float fuelUsed;                     // l
    float maxTorqueNm;
    bool doubleTorqueScale;
    bool useAbsoluteDistanceForAverage;
    bool fuelAverageUpdated;
    float distLast;
    float distZero;
    uint32_t timeLast;
};

struct Break1 {
    bool breakErr;                      // Alias: brake warning lamp.
    bool brakeWarningLamp;
    bool brakePedalActive;

    bool absErr;                        // Alias: ABS warning lamp.
    bool absLamp;
    bool absMessageTimeout;
    uint32_t absWarnActiveTime;
    bool showAbsFullscreenWarn;
    bool absFullscreenWarnCollapsed;
    bool absDiag;
};

struct Break2 {
    float totalDistance;                // meters
    int16_t impulses;
    uint32_t totalImpulses;

    float startDistance;
    uint32_t timeStart;
    uint32_t timeLast;
    bool reached400m;
    float timeAccel400m;
    bool initialized;
};

struct Kombi1 {
    bool coolantHot;
    bool oilPressureSt;             // Oil pressure switch/status from Kombi. In logs this is active during normal engine run; do not use as a STOP warning by itself.
    bool oilPressureDyn;            // true = динамическая ошибка давления масла. Несоответствие давления оборотам/нагрузке → риск отказа смазки.
    bool coolantLow;                // true = низкий уровень охлаждающей жидкости. Датчик уровня фиксирует недостаток → возможный перегрев.
    bool driverDoorOpen;
    bool handBrakeActive;

    bool tankEmpty;                 // Alias: low fuel warning lamp.
    bool tankWarn;
    bool tankError;
    uint8_t tank;
    uint32_t tankWarnActiveTime;
    bool showTankFullscreenWarn;
    bool tankFullscreenWarnCollapsed; 

    //SPEED    
    float speed;
    float avgSpeed;
    uint32_t timeLast;
    float sumSpeedTime;
    float sumTime;
    float timeAccel100;
    bool reachedSpeed100;
    uint32_t timeStart;
    uint32_t accelStartTime;
};

struct Kombi2 {
    float outsideTemp;
    bool validOutsideTemp;
};

struct Kombi3 {
    uint32_t odometer;
    bool standTimeError;
    uint32_t standTime;
    uint32_t idleTime;
};

struct Airbag1 {
    bool airbagErr;                     // Alias: airbag warning lamp.
    bool airbagLamp;
    uint32_t airWarnActiveTime;
    bool showAirbagFullscreenWarn;
    bool airbagFullscreenWarnCollapsed;
};

struct ChangesState {
    bool fuelRate;
    bool fuelAverage;
    bool fuelUsed;
    bool tank;
    bool rpm;
    bool torque;
    bool coolantTemp;
    bool outsideTemp;
    bool speed;
    bool avgSpeed;
    bool distance;
    bool timeAccel100;
    bool timeAccel400m;
    bool powerHp;
    bool odometer;

    // test data
    bool impulses;
};

struct ErrorState {
    uint8_t errorsCount;
    uint16_t interval = 5000;
};

extern ChangesState changes_status;
extern ErrorState error_state;

void parseEngine1(uint8_t buf[8], Engine1 &s);
void parseEngine2(uint8_t buf[8], Engine2 &s);
void parseEngine5(uint8_t buf[8], Engine5 &s, uint32_t time_now, float distanceSinceLastKm);
void resetEngine5TripBaseline(Engine5 &s);
void parseBreak1(uint8_t buf[8], Break1 &s);
void updateAbsWarningState(Break1 &s);
void parseBreak2(uint8_t buf[8], Break2 &s, uint32_t time_now, bool measurement_mode);
void resetBreak2TripDistance(Break2 &s);
void parseKombi1(uint8_t buf[8], Kombi1 &s, uint32_t time_now, bool measurement_mode);
void resetKombi1AverageBaseline(Kombi1 &s);
void parseKombi2(uint8_t buf[8], Kombi2 &s);
void parseKombi3(uint8_t buf[8], Kombi3 &s);
void parseAirbag1(uint8_t buf[8], Airbag1 &s, uint32_t time_now);
bool diffInt8(uint8_t a, uint8_t b);
bool diffInt16(uint16_t a, uint16_t b);
bool diffInt32(uint32_t a, uint32_t b);
bool diffFloat(float a, float b, uint8_t decimals);
