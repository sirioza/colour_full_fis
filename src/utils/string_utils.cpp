#include <Arduino.h>
#include "string_utils.h"

static uint32_t pow10u(uint8_t precision) {
  static const uint32_t factors[] = { 1, 10, 100, 1000, 10000, 100000 };
  if (precision < sizeof(factors) / sizeof(factors[0])) {
    return factors[precision];
  }

  return factors[(sizeof(factors) / sizeof(factors[0])) - 1];
}

static uint8_t countDigits(uint32_t value) {
  uint8_t digits = 1;
  while (value >= 10) {
    value /= 10;
    digits++;
  }
  return digits;
}

static void writeUnsigned(char* buffer, size_t bufSize, uint32_t value) {
  if (bufSize == 0) {
    return;
  }

  char tmp[11];
  uint8_t pos = 0;
  do {
    tmp[pos++] = (char)('0' + (value % 10));
    value /= 10;
  } while (value != 0 && pos < sizeof(tmp));

  size_t out = 0;
  while (pos > 0 && out + 1 < bufSize) {
    buffer[out++] = tmp[--pos];
  }
  buffer[out] = '\0';
}

static void appendChar(char* buffer, size_t bufSize, size_t& out, char value) {
  if (out + 1 < bufSize) {
    buffer[out++] = value;
    buffer[out] = '\0';
  }
}

static void appendText(char* buffer, size_t bufSize, size_t& out, const char* text) {
  for (size_t i = 0; text[i] != '\0'; i++) {
    appendChar(buffer, bufSize, out, text[i]);
  }
}

static void appendPaddedUnsigned(char* buffer, size_t bufSize, size_t& out, uint32_t value, uint8_t width, char pad) {
  uint8_t digits = countDigits(value);
  while (digits < width) {
    appendChar(buffer, bufSize, out, pad);
    digits++;
  }

  char intBuf[11];
  writeUnsigned(intBuf, sizeof(intBuf), value);
  appendText(buffer, bufSize, out, intBuf);
}

void valueToStrInt(char* buffer, size_t bufSize, int32_t value, uint8_t digits) {
  if (bufSize == 0) {
    return;
  }

  bool negative = value < 0;
  uint32_t absValue = negative ? (uint32_t)(-value) : (uint32_t)value;
  if (countDigits(absValue) > digits) {
    value = -1;
    negative = true;
    absValue = 1;
  }

  size_t out = 0;
  if (negative && out + 1 < bufSize) {
    buffer[out++] = '-';
  }

  writeUnsigned(buffer + out, bufSize - out, absValue);
}

void valueToStr(char* buffer, size_t bufSize, float value, int precision, uint8_t digits) {
  if (bufSize == 0) {
    return;
  }

  if (precision <= 0) {
    int32_t rounded = (int32_t)(value >= 0.0f ? value + 0.5f : value - 0.5f);
    valueToStrInt(buffer, bufSize, rounded, digits);
    return;
  }

  uint32_t factor = pow10u((uint8_t)precision);
  int32_t scaled = (int32_t)(value >= 0.0f ? value * (float)factor + 0.5f : value * (float)factor - 0.5f);
  bool negative = scaled < 0;
  uint32_t absScaled = negative ? (uint32_t)(-scaled) : (uint32_t)scaled;
  uint32_t intPart = absScaled / factor;
  uint32_t fracPart = absScaled % factor;

  if (countDigits(intPart) > digits) {
    negative = true;
    intPart = 1;
    fracPart = 0;
  }

  size_t out = 0;
  if (negative && out + 1 < bufSize) {
    buffer[out++] = '-';
  }

  char intBuf[11];
  writeUnsigned(intBuf, sizeof(intBuf), intPart);
  for (size_t i = 0; intBuf[i] != '\0' && out + 1 < bufSize; i++) {
    buffer[out++] = intBuf[i];
  }

  if (out + 1 < bufSize) {
    buffer[out++] = '.';
  }

  uint32_t divisor = factor / 10;
  while (divisor > 0 && out + 1 < bufSize) {
    buffer[out++] = (char)('0' + ((fracPart / divisor) % 10));
    divisor /= 10;
  }

  buffer[out] = '\0';
}

void formatTimeMs(uint32_t ms, char* out, size_t outSize) {
  if (outSize == 0) {
    return;
  }

	uint32_t total_seconds = ms / 1000;
	uint32_t hours = total_seconds / 3600;
	uint32_t minutes = (total_seconds % 3600) / 60;
  size_t pos = 0;
  out[0] = '\0';

	if (hours == 0) {
    for (uint8_t i = 0; i < 9; i++) {
      appendChar(out, outSize, pos, ' ');
    }
    appendPaddedUnsigned(out, outSize, pos, minutes, 2, '0');
    appendText(out, outSize, pos, " min");
	} else if(hours < 10){
    appendPaddedUnsigned(out, outSize, pos, hours, 4, ' ');
    appendText(out, outSize, pos, " h ");
    appendPaddedUnsigned(out, outSize, pos, minutes, 2, '0');
    appendText(out, outSize, pos, " min");
	} else {
    appendPaddedUnsigned(out, outSize, pos, hours, 3, ' ');
    appendText(out, outSize, pos, " h ");
    appendPaddedUnsigned(out, outSize, pos, minutes, 2, '0');
    appendText(out, outSize, pos, " min");
	}
}
