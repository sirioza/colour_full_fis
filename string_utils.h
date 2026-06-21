#pragma once
#include <stddef.h>
#include <stdint.h>

void valueToStr(char* buffer, size_t bufSize, float value, int precision = 1, uint8_t digits = 4);
void valueToStrInt(char* buffer, size_t bufSize, int32_t value, uint8_t digits = 4);
void formatTimeMs(uint32_t ms, char* out, size_t outSize);
