#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Fonts/FreeSans9pt7b.h>
#include <SPI.h>
#include <cstdint>
#include "config.h"
#include "icons.h"
#include "graphics.h"
#include "screens/screen_fuel.h"
#include "screens/screen_engine.h"
#include "screens/screen_speed.h"
#include "screens/screen_dynamics.h"
#include "screens/screen_test.h"
#include "screens/screen_warnings.h"
#include "trip_history/persistent_state.h"
#include "trip_history/trip_memory.h"
#include "utils/vehicle_utils.h"
#include "utils/string_utils.h"

#ifdef MOCK_CAN
#include "mock_can.h"
#else
#include "driver/twai.h"
#endif

enum Screen : int8_t {
  SCREEN_TEST = 1,
  SCREEN_FUEL,
  SCREEN_ENGINE,
  SCREEN_SPEED,
  SCREEN_DYNAMICS,
  FIRST_SCREEN = SCREEN_TEST,
  LAST_SCREEN = SCREEN_DYNAMICS,
  SCREEN_WARNING = 100
};

Adafruit_ST7789 tft(TFT_CS_PIN, TFT_DC_PIN, TFT_RST_PIN);
Graphics gfx(tft);

struct ScreenState {
  int8_t position = FIRST_SCREEN ;
  int8_t previousPosition = FIRST_SCREEN;
  bool redraw = true;
  uint32_t lastRefreshMs = 0;
  bool headerSeparatorDrawn = false;
};

struct DisplayCache {
  uint16_t dteDisplayOld = 0;
  bool dteOldValid = false;
  bool dteDrawn = false;
  bool travelTimeDrawn = false;
  uint32_t lastTravelTimeMinute = UINT32_MAX;
  int16_t distancePrevious = -1;
  uint8_t statusIcons = 0;
};

struct WarningUiState {
  uint32_t previousMillis = 0;
  int8_t currentIdx = -1;
  int8_t drawnIdx = -1;
  bool next = false;
};

ScreenState screenState;
DisplayCache displayCache;
WarningUiState warningUi;

Engine1 engine1;
Engine2 engine2;
Engine5 engine5;
Break1 break1;
Break2 break2;
Kombi1 kombi1;
Kombi2 kombi2;
Kombi3 kombi3;
Airbag1 airbag1;

OdometerMemoryState odometerMemory;
DteMemoryState dteMemory;

const uint32_t DTE_STOP_SAVE_INTERVAL_MS = 30000UL;
const float ODOMETER_SAVE_SPEED_KMH = 5.0f;
const float STOP_SPEED_KMH = 0.5f;
const float DTE_LONG_TERM_ALPHA = 0.02f;

const uint16_t CAN_BUS_MESSAGE_TIMEOUT_MS = 1500;
const uint16_t ABS_WARNING_STARTUP_GRACE_MS = 7000;
const uint16_t NON_CRITICAL_FULLSCREEN_WARNING_MS = 5000;
uint32_t lastCanMessageTime = 0;
uint32_t lastBreak1MessageTime = 0;
bool canBusFault = false;

const uint8_t TITLE_INDENT = 15;
const uint8_t MEASUREMENT_INDENT = 190;
const uint8_t VALUE_INDENT = 183;
const uint8_t BLOCK_INDENT = 15;

const float DTE_MAX_KM = 999.0f;
const uint8_t DTE_DISPLAY_STEP_KM = 10;
uint32_t lastDteStoppedSaveMs = 0;
bool odometerCanSeen = false;
bool tankCanSeen = false;
bool engine5FuelAverageUpdatedSinceLastDte = false;
bool odometerBelow5Saved = false;
bool odometerFullStopSaved = false;
const int16_t TRAVEL_TIME_VALUE_X = VALUE_INDENT - 75;
const uint8_t TRAVEL_TIME_VALUE_CLEAR_W = 115;

//Warnings
const uint8_t Y_ICON = 70;
const uint16_t Y_MAIN_TEXT = 240;
const uint16_t Y_SECOND_TEXT = 270;

void applyScreenCommand(int8_t newPosition) {
  if (newPosition == screenState.position) {
    return;
  }

  screenState.position = newPosition;
	screenState.redraw = true;
  warningUi.previousMillis = 0;
  warningUi.currentIdx = -1;
  screenState.lastRefreshMs = 0;
}

void switchScreen(int8_t direction) {
  if (screenState.position == SCREEN_WARNING) {
    return;
  }

  int8_t screen = screenState.position;

  if (screen < FIRST_SCREEN || screen > LAST_SCREEN) {
    screen = (direction > 0) ? FIRST_SCREEN : LAST_SCREEN;
  } else {
    screen += direction;

    if (screen > LAST_SCREEN) screen = FIRST_SCREEN;
    if (screen < FIRST_SCREEN) screen = LAST_SCREEN;
  }

  applyScreenCommand(screen);
}

void handleButtons() {
  static bool nextPressed = false;
  static bool prevPressed = false;

  bool nextState = digitalRead(BTN_NEXT);
  bool prevState = digitalRead(BTN_PREV);

  if (!nextPressed && nextState == LOW) {
    nextPressed = true;
    switchScreen(+1);
  }

  if (nextState == HIGH) {
    nextPressed = false;
  }

  if (!prevPressed && prevState == LOW) {
    prevPressed = true;
    switchScreen(-1);
  }

  if (prevState == HIGH) {
    prevPressed = false;
  }
}

void clearDrawnWarningContent() {
  if (warningUi.drawnIdx < 0) {
    return;
  }

  gfx.drawScreenFromStrip(48, 295);
  warningUi.drawnIdx = -1;
}

void drawTravelTimeValue() {
  char travelTimeBuf[20];
  formatTimeMs(tripMemory.travelTimeMs, travelTimeBuf, sizeof(travelTimeBuf));
  gfx.clearTextOverBackground(TRAVEL_TIME_VALUE_X, 150 + BLOCK_INDENT - 16, TRAVEL_TIME_VALUE_CLEAR_W, 22, 0, 0);
  gfx.drawText(travelTimeBuf, TRAVEL_TIME_VALUE_X, 150 + BLOCK_INDENT, ILI9341_WHITE);
}

void markTripChanged() {
  displayCache.distancePrevious = -1;
  displayCache.lastTravelTimeMinute = 0xFFFFFFFFUL;
  displayCache.travelTimeDrawn = false;
  changes_status.fuelAverage = true;
  changes_status.fuelUsed = true;
  changes_status.distance = true;
  changes_status.avgSpeed = true;
}

void loadPersistentState(uint32_t currentMillis) {
  persistentStateBegin();

  tripMemoryBegin(currentMillis);

  if (loadOdometerMemory(odometerMemory) && odometerMemory.valid) {
    kombi3.odometer = odometerMemory.odometerKm;
    changes_status.odometer = true;
  } else {
    odometerMemory.valid = false;
  }

  if (!loadDteMemory(dteMemory)) {
    dteMemory.valid = false;
  }

  markTripChanged();
}

void updateDteMemory(uint32_t currentMillis) {
  if (tripMemoryAwaitingStandTime()) {
    engine5FuelAverageUpdatedSinceLastDte = false;
    return;
  }

  bool averageUpdated = engine5FuelAverageUpdatedSinceLastDte && engine5.fuelAverage > 0.0f;
  if (averageUpdated) {
    if (!dteMemory.valid || dteMemory.longTermAverageFuel <= 0.0f) {
      dteMemory.longTermAverageFuel = engine5.fuelAverage;
    } else {
      dteMemory.longTermAverageFuel += (engine5.fuelAverage - dteMemory.longTermAverageFuel) * DTE_LONG_TERM_ALPHA;
    }
  }
  engine5FuelAverageUpdatedSinceLastDte = false;

  bool canCalculateDte = tankCanSeen && !kombi1.tankError && dteMemory.longTermAverageFuel > 0.0f;
  if (canCalculateDte) {
    dteMemory.valid = true;
    dteMemory.tankLiters = kombi1.tank;
    dteMemory.dteKm = ((float)kombi1.tank / dteMemory.longTermAverageFuel) * 100.0f;
    if (dteMemory.dteKm > DTE_MAX_KM) {
      dteMemory.dteKm = DTE_MAX_KM;
    }
  }

  bool stopped = kombi1.speed <= STOP_SPEED_KMH;
  if (!stopped) {
    lastDteStoppedSaveMs = 0;
    return;
  }

  if (!dteMemory.valid) {
    return;
  }

  if (lastDteStoppedSaveMs == 0 || (uint32_t)(currentMillis - lastDteStoppedSaveMs) >= DTE_STOP_SAVE_INTERVAL_MS) {
    saveDteMemory(dteMemory);
    lastDteStoppedSaveMs = currentMillis;
  }
}

void updateOdometerMemory() {
  if (!odometerCanSeen) {
    return;
  }

  odometerMemory.valid = true;
  odometerMemory.odometerKm = kombi3.odometer;

  if (kombi1.speed >= ODOMETER_SAVE_SPEED_KMH) {
    odometerBelow5Saved = false;
    odometerFullStopSaved = false;
    return;
  }

  if (!odometerBelow5Saved) {
    saveOdometerMemory(odometerMemory);
    odometerBelow5Saved = true;
  }

  if (kombi1.speed <= STOP_SPEED_KMH && !odometerFullStopSaved) {
    saveOdometerMemory(odometerMemory);
    odometerFullStopSaved = true;
  }
}

uint16_t roundDteForDisplay(float dteKm) {
  if (dteKm <= 0.0f) {
    return 0;
  }

  uint16_t wholeKm = (uint16_t)dteKm;
  uint8_t remainder = wholeKm % DTE_DISPLAY_STEP_KM;
  uint16_t rounded = wholeKm - remainder;
  if (remainder > DTE_DISPLAY_STEP_KM / 2) {
    rounded += DTE_DISPLAY_STEP_KM;
  }

  return rounded;
}

void updatePersistentState(uint32_t currentMillis) {
  if (updateTripMemory(engine5, break2, kombi1, currentMillis)) {
    markTripChanged();
  }
  updateDteMemory(currentMillis);
  updateOdometerMemory();
}

void collapseExpiredNonCriticalFullscreenWarnings(uint32_t currentMillis) {
  if (kombi1.showTankFullscreenWarn && kombi1.tankWarnActiveTime != 0 &&
      (uint32_t)(currentMillis - kombi1.tankWarnActiveTime) >= NON_CRITICAL_FULLSCREEN_WARNING_MS) {
    kombi1.showTankFullscreenWarn = false;
    kombi1.tankFullscreenWarnCollapsed = true;
  }

  if (airbag1.showAirbagFullscreenWarn && airbag1.airWarnActiveTime != 0 &&
      (uint32_t)(currentMillis - airbag1.airWarnActiveTime) >= NON_CRITICAL_FULLSCREEN_WARNING_MS) {
    airbag1.showAirbagFullscreenWarn = false;
    airbag1.airbagFullscreenWarnCollapsed = true;
  }

  if (break1.showAbsFullscreenWarn && break1.absWarnActiveTime != 0 &&
      (uint32_t)(currentMillis - break1.absWarnActiveTime) >= NON_CRITICAL_FULLSCREEN_WARNING_MS) {
    break1.showAbsFullscreenWarn = false;
    break1.absFullscreenWarnCollapsed = true;
  }
}

void startNonCriticalFullscreenWarningTimer(int8_t warnIdx, uint32_t currentMillis) {
  switch (warnIdx) {
    case 7:
      if (kombi1.showTankFullscreenWarn && kombi1.tankWarnActiveTime == 0) {
        kombi1.tankWarnActiveTime = currentMillis;
      }
      break;
    case 8:
      if (airbag1.showAirbagFullscreenWarn && airbag1.airWarnActiveTime == 0) {
        airbag1.airWarnActiveTime = currentMillis;
      }
      break;
    case 9:
      if (break1.showAbsFullscreenWarn && break1.absWarnActiveTime == 0) {
        break1.absWarnActiveTime = currentMillis;
      }
      break;
    default:
      break;
  }
}

void initWarnings(uint32_t currentMillis) {
  const uint8_t numWarnings = 12;
  uint16_t warningsMask = 0;

  collapseExpiredNonCriticalFullscreenWarnings(currentMillis);

  if (canBusFault) warningsMask |= (1 << 0);
  if (break1.breakErr) warningsMask |= (1 << 1);
  if (kombi1.oilPressureDyn) warningsMask |= (1 << 2);
  if (kombi1.coolantHot) warningsMask |= (1 << 3);
  if (kombi1.coolantLow) warningsMask |= (1 << 4);
  if (kombi1.handBrakeActive && kombi1.speed >= 5.0f) warningsMask |= (1 << 5);
  if (kombi1.driverDoorOpen) warningsMask |= (1 << 6);
  if (kombi1.tankEmpty && kombi1.showTankFullscreenWarn) warningsMask |= (1 << 7);
  if (airbag1.airbagErr && airbag1.showAirbagFullscreenWarn) warningsMask |= (1 << 8);
  if (currentMillis >= ABS_WARNING_STARTUP_GRACE_MS && break1.absErr && break1.showAbsFullscreenWarn) warningsMask |= (1 << 9);
  if (engine2.coolantError) warningsMask |= (1 << 10);

	// hide warning diplay and to redirect to previous display
	if (warningsMask == 0) {
		if (warningUi.currentIdx != -1) {
			screenState.redraw = true;
			screenState.position = screenState.previousPosition;
		}
    warningUi.currentIdx = -1;
		return; 
  }

	// to applay warning display
  if (screenState.position != SCREEN_WARNING) {
    screenState.redraw = true;
    screenState.previousPosition = screenState.position;
  }
  screenState.position = SCREEN_WARNING;

	// set warning index 
	bool timeElapsed = (currentMillis - warningUi.previousMillis >= error_state.interval);
	bool currentWarnActive = (warningUi.currentIdx != -1 && (warningsMask & (1 << warningUi.currentIdx)));
	error_state.errorsCount = 0;
	for (uint8_t i = 0; i < numWarnings; i++) {
		if (warningsMask & (1 << i)) {
			error_state.errorsCount++;
		}
	}

	if (!currentWarnActive || (timeElapsed && error_state.errorsCount > 1)) {
		warningUi.previousMillis = currentMillis;
		int8_t startIndex = (warningUi.currentIdx + 1) % numWarnings;
		for (uint8_t i = 0; i < numWarnings; i++) {
			int8_t checkIndex = (startIndex + i) % numWarnings;
			if (warningsMask & (1 << checkIndex)) {
				warningUi.currentIdx = checkIndex;
				warningUi.next = true;
				break;
			}
		}
	}

  startNonCriticalFullscreenWarningTimer(warningUi.currentIdx, currentMillis);
}

void mainDisplay(uint32_t currentMillis) {
	if (!screenState.redraw && (uint32_t)(currentMillis - screenState.lastRefreshMs) <= REFRESH_SCREEN_TIME) {
		return;
	}

  uint8_t newState = 0;
  if(screenState.position != SCREEN_WARNING) {
    if (kombi1.tankEmpty && !kombi1.showTankFullscreenWarn) {
      newState |= BIT_FUEL;
    }
    if (break1.absErr && !break1.showAbsFullscreenWarn) {
      newState |= BIT_ABS;
    }
    if (airbag1.airbagErr && !airbag1.showAirbagFullscreenWarn) {
      newState |= BIT_AIR;
    }
    if (kombi2.validOutsideTemp && kombi2.outsideTemp >= -7.0f && kombi2.outsideTemp <= 4.0f) {
      newState |= BIT_ICE;
    }
  }

  bool statusChanged = screenState.position != SCREEN_WARNING  && newState != displayCache.statusIcons;
  bool screenChanged = screenState.redraw;

  switch (screenState.position) {
    case SCREEN_FUEL:
      screenChanged = screenChanged || changes_status.fuelRate || changes_status.fuelAverage || changes_status.fuelUsed || changes_status.tank;
      break;
    case SCREEN_ENGINE:
      screenChanged = screenChanged || changes_status.rpm || changes_status.torque || changes_status.powerHp || changes_status.coolantTemp || changes_status.outsideTemp;
      break;
    case SCREEN_SPEED: {
      int16_t distance = tripMemory.distanceMeters / 1000.0f;
      uint32_t travelTimeMinute = tripMemory.travelTimeMs / 60000UL;
      screenChanged = screenChanged || changes_status.speed || changes_status.avgSpeed || changes_status.distance || changes_status.odometer || diffInt16(distance, displayCache.distancePrevious) || displayCache.distancePrevious < 0 || !displayCache.travelTimeDrawn || travelTimeMinute != displayCache.lastTravelTimeMinute;
      break;
    }
    case SCREEN_DYNAMICS:
      screenChanged = screenChanged || changes_status.timeAccel100 || changes_status.timeAccel400m;
      break;
    case SCREEN_TEST:
      screenChanged = screenChanged || changes_status.distance;
      break;      
    case SCREEN_WARNING:
      screenChanged = screenChanged || warningUi.next;
      break;
    default:
      break;
  }

  if (!screenChanged && !statusChanged) {
    screenState.lastRefreshMs = currentMillis;
    return;
  }

  ScreenLayoutContext screenLayout = { gfx, tft, TITLE_INDENT, MEASUREMENT_INDENT, BLOCK_INDENT };
  char valueBuf[10];
  
  switch (screenState.position) {
    case SCREEN_FUEL: {
      if (screenState.redraw) {
        gfx.drawScreenBackgroundKeepingHeaderSeparator(screenState.headerSeparatorDrawn);
        drawFuelScreenLayout(screenLayout, changes_status, displayCache.dteDrawn);
        screenState.redraw = false;
      }

      if (changes_status.fuelRate) {
        valueToStr(valueBuf, sizeof(valueBuf), engine5.fuelRate);
        gfx.drawRightAlignedText(valueBuf, VALUE_INDENT, 60 + BLOCK_INDENT, ILI9341_WHITE, WIDTH_000_0_VAL);
        changes_status.fuelRate = false;
      }

      if (changes_status.fuelAverage) {
        if(tripMemory.averageFuel != 0.0f) {
          valueToStr(valueBuf, sizeof(valueBuf), tripMemory.averageFuel, 1);
          gfx.drawRightAlignedText(valueBuf, VALUE_INDENT, 90 + BLOCK_INDENT, ILI9341_WHITE, WIDTH_000_0_VAL);
        } else {
          gfx.drawRightAlignedText("--.-", VALUE_INDENT, 90 + BLOCK_INDENT, ILI9341_WHITE, WIDTH_000_0_VAL);
        }
        
        changes_status.fuelAverage = false;
      }

      bool dteValid = dteMemory.valid && dteMemory.longTermAverageFuel > 0.0f;
      uint16_t dteDisplayCurrent = roundDteForDisplay(dteMemory.dteKm);

      if (!displayCache.dteDrawn || dteValid != displayCache.dteOldValid || (dteValid && diffInt16(dteDisplayCurrent, displayCache.dteDisplayOld))) {
        if (dteValid) {
          valueToStrInt(valueBuf, sizeof(valueBuf), dteDisplayCurrent);
          gfx.drawRightAlignedText(valueBuf, VALUE_INDENT, 120 + BLOCK_INDENT, ILI9341_WHITE, WIDTH_0000_VAL);
          displayCache.dteDisplayOld = dteDisplayCurrent;
        } else {
          gfx.drawRightAlignedText("---", VALUE_INDENT, 120 + BLOCK_INDENT, ILI9341_WHITE, WIDTH_0000_VAL);
        }
        displayCache.dteOldValid = dteValid;
        displayCache.dteDrawn = true;
      }

      if (changes_status.fuelUsed) {
        valueToStr(valueBuf, sizeof(valueBuf), tripMemory.fuelUsed, 2);
        gfx.drawRightAlignedText(valueBuf, VALUE_INDENT, 150 + BLOCK_INDENT, ILI9341_WHITE, WIDTH_000_0_VAL);
        changes_status.fuelUsed = false;
      }

      if (changes_status.tank) {
        valueToStrInt(valueBuf, sizeof(valueBuf), kombi1.tank);
        gfx.drawRightAlignedText(valueBuf, VALUE_INDENT, 180 + BLOCK_INDENT, kombi1.tankEmpty ? ILI9341_RED : ILI9341_WHITE, WIDTH_0000_VAL);
        changes_status.tank = false;
      }
      break;
    }
    case SCREEN_ENGINE: {
      if (screenState.redraw) {
        gfx.drawScreenBackgroundKeepingHeaderSeparator(screenState.headerSeparatorDrawn);
        drawEngineScreenLayout(screenLayout, changes_status);
        screenState.redraw = false;
      }

      if (changes_status.rpm) {
        valueToStrInt(valueBuf, sizeof(valueBuf), engine1.rpm);
        gfx.drawRightAlignedText(valueBuf, VALUE_INDENT, 60 + BLOCK_INDENT, ILI9341_WHITE, WIDTH_0000_VAL);
        changes_status.rpm = false;
      }

      if (changes_status.torque) {
        valueToStr(valueBuf, sizeof(valueBuf), engine1.torque, 0);
        gfx.drawRightAlignedText(valueBuf, VALUE_INDENT, 90 + BLOCK_INDENT, ILI9341_WHITE, WIDTH_0000_VAL);
        changes_status.torque = false;
      }
      
      if (changes_status.powerHp) {
        valueToStr(valueBuf, sizeof(valueBuf), engine1.powerHp, 0);
        gfx.drawRightAlignedText(valueBuf, VALUE_INDENT, 120 + BLOCK_INDENT, ILI9341_WHITE, WIDTH_0000_VAL);
        changes_status.powerHp = false;
      }

      if (changes_status.coolantTemp) {
        valueToStrInt(valueBuf, sizeof(valueBuf), engine2.coolantTemp);
        gfx.drawRightAlignedText(valueBuf, VALUE_INDENT, 150 + BLOCK_INDENT, ILI9341_WHITE, WIDTH_0000_VAL);
        changes_status.coolantTemp = false;
      }

      if (changes_status.outsideTemp) {
        valueToStr(valueBuf, sizeof(valueBuf), kombi2.outsideTemp);
        gfx.drawRightAlignedText(valueBuf, VALUE_INDENT, 180 + BLOCK_INDENT, ILI9341_WHITE, WIDTH_000_0_VAL);
        changes_status.outsideTemp = false;
      }
      break;
    }
    case SCREEN_SPEED: {
      if (screenState.redraw) {
        gfx.drawScreenBackgroundKeepingHeaderSeparator(screenState.headerSeparatorDrawn);
        drawSpeedScreenLayout(screenLayout, changes_status, displayCache.travelTimeDrawn);
        screenState.redraw = false;
      }

      if (changes_status.speed) {
        valueToStr(valueBuf, sizeof(valueBuf), kombi1.speed, 0);
        gfx.drawRightAlignedText(valueBuf, VALUE_INDENT, 60 + BLOCK_INDENT, ILI9341_WHITE, WIDTH_0000_VAL);
        changes_status.speed = false;
      }

      if (changes_status.avgSpeed) {
        valueToStr(valueBuf, sizeof(valueBuf), tripMemory.averageSpeed, 0);
        gfx.drawRightAlignedText(valueBuf, VALUE_INDENT, 90 + BLOCK_INDENT, ILI9341_WHITE, WIDTH_0000_VAL);
        changes_status.avgSpeed = false;
      }

      int16_t distance = tripMemory.distanceMeters / 1000.0f;
      if (changes_status.distance || diffInt16(distance, displayCache.distancePrevious) || displayCache.distancePrevious < 0) {
        valueToStrInt(valueBuf, sizeof(valueBuf), distance);
        gfx.drawRightAlignedText(valueBuf, VALUE_INDENT, 120 + BLOCK_INDENT, ILI9341_WHITE, WIDTH_0000_VAL);
        displayCache.distancePrevious = distance;
        changes_status.distance = false;
      }
      
      uint32_t travelTimeMinute = tripMemory.travelTimeMs / 60000UL;
      if (!displayCache.travelTimeDrawn || travelTimeMinute != displayCache.lastTravelTimeMinute) {
        displayCache.lastTravelTimeMinute = travelTimeMinute;
        drawTravelTimeValue();
        displayCache.travelTimeDrawn = true;
      }

      if (changes_status.odometer) {
        valueToStrInt(valueBuf, sizeof(valueBuf), kombi3.odometer, 6);
        gfx.drawRightAlignedText(valueBuf, VALUE_INDENT, 180 + BLOCK_INDENT, ILI9341_WHITE, WIDTH_000000_VAL);
        changes_status.odometer = false;
      }
      break;
    }
    case SCREEN_DYNAMICS: {
      if (screenState.redraw) {
        gfx.drawScreenBackgroundKeepingHeaderSeparator(screenState.headerSeparatorDrawn);
        drawDynamicsScreenLayout(screenLayout, changes_status);
        screenState.redraw = false;
      }

      if (changes_status.timeAccel100) {
        valueToStr(valueBuf, sizeof(valueBuf), kombi1.timeAccel100);
        gfx.drawRightAlignedText(valueBuf, VALUE_INDENT, 60 + BLOCK_INDENT, ILI9341_WHITE, WIDTH_000_0_VAL);
        changes_status.timeAccel100 = false;
      }

      if (changes_status.timeAccel400m) {
        valueToStr(valueBuf, sizeof(valueBuf), break2.timeAccel400m);
        gfx.drawRightAlignedText(valueBuf, VALUE_INDENT, 90 + BLOCK_INDENT, ILI9341_WHITE, WIDTH_000_0_VAL);
        changes_status.timeAccel400m = false;
      }
      break;
    }
    case SCREEN_WARNING: {
      if (screenState.redraw) {
        gfx.drawScreenBackgroundKeepingHeaderSeparator(screenState.headerSeparatorDrawn);
        gfx.drawScreenFromStrip(295);
        displayCache.statusIcons = 0;
        drawWarningsScreenLayout(screenLayout);
        warningUi.drawnIdx = -1;
        screenState.redraw = false;
      }				

      if (warningUi.next) {
        clearDrawnWarningContent();

        switch (warningUi.currentIdx) {
          case 0:
            gfx.drawWarningText("CAN BUS", 155, ILI9341_YELLOW, 2);
            gfx.drawWarningText("FAULT", 190, ILI9341_YELLOW, 2);
            gfx.drawWarningText("Data unavailable.", 235, ILI9341_YELLOW);
            break;
          case 1:
            gfx.drawBrakeWarningIcon(50, Y_ICON + 8, ILI9341_RED);
            gfx.drawWarningText("STOP!", Y_MAIN_TEXT, ILI9341_RED, 2);
            gfx.drawWarningText("Check brake fluid.", Y_SECOND_TEXT, ILI9341_RED);
            break;
          case 2:
            gfx.drawOilPressureWarningIcon(13, Y_ICON + 8, ILI9341_RED);
            gfx.drawWarningText("STOP!", Y_MAIN_TEXT, ILI9341_RED, 2);
            gfx.drawWarningText("Stop engine. Check oil level.", Y_SECOND_TEXT, ILI9341_RED);
            break;
          case 3:
            gfx.drawCoolantWarningIcon(50, Y_ICON + 8, ILI9341_RED);
            gfx.drawWarningText("STOP!", Y_MAIN_TEXT, ILI9341_RED, 2);
            gfx.drawWarningText("Stop engine. Coolant hot.", Y_SECOND_TEXT, ILI9341_RED);
            break;
          case 4:
            gfx.drawCoolantWarningIcon(50, Y_ICON + 8, ILI9341_RED);
            gfx.drawWarningText("STOP!", Y_MAIN_TEXT, ILI9341_RED, 2);
            gfx.drawWarningText("Check coolant manually.", Y_SECOND_TEXT, ILI9341_RED);
            break;
          case 5:
            gfx.drawHandbrakeWarningIcon(50, Y_ICON + 8, ILI9341_RED);
            gfx.drawWarningText("WARNING!", Y_MAIN_TEXT, ILI9341_RED, 2);
            gfx.drawWarningText("Handbrake active.", Y_SECOND_TEXT, ILI9341_RED);
            break;
          case 6:
            gfx.drawDoorWarningIcon(50, Y_ICON - 6, ILI9341_RED);
            gfx.drawWarningText("WARNING!", Y_MAIN_TEXT, ILI9341_RED, 2);
            gfx.drawWarningText("Driver door open.", Y_SECOND_TEXT, ILI9341_RED);
            break;
          case 7:
            gfx.drawFuelWarningIcon(50, Y_ICON - 13, ILI9341_YELLOW);
            gfx.drawWarningText("WARNING!", Y_MAIN_TEXT, ILI9341_YELLOW, 2);
            gfx.drawWarningText("Please refuel", Y_SECOND_TEXT, ILI9341_YELLOW);
            break;
          case 8:
            gfx.drawAirbagWarningIcon(50, Y_ICON + 8, ILI9341_YELLOW);
            gfx.drawWarningText("WARNING!", Y_MAIN_TEXT, ILI9341_YELLOW, 2);
            gfx.drawWarningText("Airbag fault.", Y_SECOND_TEXT, ILI9341_YELLOW);
            break;
          case 9:
            gfx.drawAbsWarningIcon(50, Y_ICON + 8, ILI9341_YELLOW);
            gfx.drawWarningText("WARNING!", Y_MAIN_TEXT, ILI9341_YELLOW, 2);
            gfx.drawWarningText(break1.absMessageTimeout ? "ABS CAN timeout." : "ABS fault.", Y_SECOND_TEXT, ILI9341_YELLOW);
            break;
          case 10:
            gfx.drawCoolantWarningIcon(50, Y_ICON + 8, ILI9341_YELLOW);
            gfx.drawWarningText("WARNING!", Y_MAIN_TEXT, ILI9341_YELLOW, 2);
            gfx.drawWarningText("Coolant sensor fault.", Y_SECOND_TEXT, ILI9341_YELLOW);
            break;
          case 11:
            gfx.drawWashWarningIcon(50, Y_ICON - 2, ILI9341_YELLOW);
            gfx.drawWarningText("WARNING!", Y_MAIN_TEXT, ILI9341_YELLOW, 2);
            gfx.drawWarningText("Top up wash fluid.", Y_SECOND_TEXT, ILI9341_YELLOW);
            break;
        }

        warningUi.drawnIdx = warningUi.currentIdx;
        warningUi.next = false;
      }
      break;
    }
    case SCREEN_TEST: {
      if (screenState.redraw) {
        gfx.drawScreenBackgroundKeepingHeaderSeparator(screenState.headerSeparatorDrawn);
        drawTestDataScreenLayout(screenLayout, changes_status);
        screenState.redraw = false;
      }

      valueToStrInt(valueBuf, sizeof(valueBuf), break2.impulses);
      gfx.drawRightAlignedText(valueBuf, VALUE_INDENT, 60 + BLOCK_INDENT, ILI9341_WHITE, WIDTH_0000_VAL);

      valueToStrInt(valueBuf, sizeof(valueBuf), tripMemory.distanceMeters);
      gfx.drawRightAlignedText(valueBuf, VALUE_INDENT, 90 + BLOCK_INDENT, ILI9341_WHITE, WIDTH_0000_VAL);

      break;
    }
    default:
      if (screenState.redraw) {
        gfx.drawScreenFromStrip();
        screenState.headerSeparatorDrawn = false;
        gfx.drawText("NOT FOUND SCREEN", 27, 30, ILI9341_WHITE);
        gfx.drawHeaderSeparator();
        screenState.headerSeparatorDrawn = true;
        screenState.redraw = false;
      };
  }

  if (screenState.position != SCREEN_WARNING && statusChanged) {
    gfx.drawScreenFromStrip(295);

    uint8_t iconX = BLOCK_INDENT;
    if (newState & BIT_FUEL) {
      gfx.drawIcon(fuelIcon, iconX, 295, ILI9341_YELLOW, 1);
      iconX += 30;
    }
    if (newState & BIT_ABS) {
      gfx.drawIcon(absIcon, iconX, 295, ILI9341_YELLOW, 1);
      iconX += 30;
    }
    if (newState & BIT_AIR) {
      gfx.drawIcon(airIcon, iconX, 295, ILI9341_YELLOW, 1);
      iconX += 30;
    }
    if (newState & BIT_ICE) {
      gfx.drawSnowflake(iconX + 14, 304, 5, 2);
    }

    displayCache.statusIcons = newState;
  }

  screenState.lastRefreshMs = currentMillis;
}

void parseCanBusData(unsigned long id, byte *buf, uint32_t time_now) {
  switch (id) {
    case ENGINE1_ID:
      parseEngine1(buf, engine1);
      break;
    case ENGINE2_ID:
      parseEngine2(buf, engine2);
      break;
    case ENGINE5_ID:
      parseEngine5(buf, engine5, time_now, break2.totalDistance);
      engine5FuelAverageUpdatedSinceLastDte = engine5FuelAverageUpdatedSinceLastDte || engine5.fuelAverageUpdated;
      break;
    case BREAK1_ID:
      lastBreak1MessageTime = time_now;
      parseBreak1(buf, break1);
      break;
    case BREAK2_ID:
      parseBreak2(buf, break2, time_now, screenState.position == SCREEN_DYNAMICS);
      break;
    case KOMBI1_ID:
      parseKombi1(buf, kombi1, time_now, screenState.position == SCREEN_DYNAMICS);
      tankCanSeen = true;
      break;
    case KOMBI2_ID:
      parseKombi2(buf, kombi2);
      break;
		 case KOMBI3_ID:
      parseKombi3(buf, kombi3);
      odometerCanSeen = true;
      if (resolveTripMemoryByStandTime(engine5, break2, kombi1, kombi3, time_now)) {
        markTripChanged();
      }
      break;
    case AIRBAG1_ID:
      parseAirbag1(buf, airbag1, time_now);
      break;
    default:
      break;
  }
}

void updateCanTimeoutWarnings(uint32_t time_now) {
	#ifdef MOCK_CAN // for the test mode
		return;
	#endif

  if (time_now < ABS_WARNING_STARTUP_GRACE_MS) {
    return;
  }

  if (lastCanMessageTime == 0) {
    canBusFault = true;
    return;
  }

  canBusFault = (time_now - lastCanMessageTime) > CAN_BUS_MESSAGE_TIMEOUT_MS;
  if (canBusFault) {
    return;
  }

  if (lastBreak1MessageTime != 0 && (time_now - lastBreak1MessageTime) <= CAN_BUS_MESSAGE_TIMEOUT_MS) {
    return;
  }

  break1.absMessageTimeout = true;
  break1.absErr = break1.absLamp || break1.absMessageTimeout;
  updateAbsWarningState(break1);
}

void printFrame(unsigned long id, byte len, const byte *buf) {
  if(id == ENGINE6_ID || id == BREAK3_ID || id == SYSTEMINFO1_ID || id == NO_FOUND_ID) return;
  Serial.print("{0x");
  Serial.print(id, HEX);
  Serial.print(", ");
  Serial.print(len);
  Serial.print(", {");
  for (byte i = 0; i < len; i++) {
    Serial.print("0x");
    if (buf[i] < 0x10) Serial.print("0");
    Serial.print(buf[i], HEX);
    if (i < len - 1) Serial.print(",");
  }
  Serial.println("}},");
}

void setup() {
  Serial.begin(115200);

  uint32_t currentMillis = millis();
  pinMode(TFT_BL_PIN, OUTPUT);
  digitalWrite(TFT_BL_PIN, HIGH);

  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);

  loadPersistentState(currentMillis);

  SPI.begin(TFT_SCLK_PIN, -1, TFT_MOSI_PIN, TFT_CS_PIN);
  tft.init(SCREEN_WIDTH, SCREEN_HEIGHT);
  tft.setSPISpeed(TFT_SPI_SPEED);
  tft.setRotation(TFT_ROTATION);
  tft.setFont(&FreeSans9pt7b);

	gfx.drawScreenFromStrip();
	gfx.drawText("WELCOME", 30, 170, ILI9341_WHITE, 0, 2);

  //CAN
	#ifndef MOCK_CAN
  twai_general_config_t generalConfig = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX_PIN, (gpio_num_t)CAN_RX_PIN, TWAI_MODE_NORMAL);
  generalConfig.rx_queue_len = 128;
  generalConfig.tx_queue_len = 0;
  twai_timing_config_t timingConfig = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t filterConfig = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  if (twai_driver_install(&generalConfig, &timingConfig, &filterConfig) == ESP_OK) {
    twai_start();
  }
  #else
  CAN.begin(0,0,0);
  #endif
}

void loop() {
  uint32_t currentMillis = millis();

  handleButtons();

  //CAN START
  #ifdef MOCK_CAN
  while (CAN.checkReceive() == CAN_MSGAVAIL) {
    unsigned long id;
    byte len;
    byte buf[8];

    CAN.readMsgBuf(&id, &len, buf);

    uint32_t now = millis();

    lastCanMessageTime = now;
    canBusFault = false;

    parseCanBusData(id, buf, now);
  }
  #else
  twai_message_t msg;
  while (twai_receive(&msg, 0) == ESP_OK) {
    if (!msg.extd && !msg.rtr) {
      uint32_t now = millis();

      lastCanMessageTime = now;
      canBusFault = false;

      parseCanBusData(msg.identifier, msg.data, now);
    }
  }
  #endif
  //CAN END

  currentMillis = millis();
  updatePersistentState(currentMillis);

  updateCanTimeoutWarnings(currentMillis);

  if (currentMillis < 3000) {
    return;
  }

  initWarnings(currentMillis);
  mainDisplay(currentMillis);
}