#pragma once
#include "string.h"

typedef enum {
  EN = 0x0,
  SV = 0x1
} Language;

void time_to_words(Language lang, int hours, int minutes, int seconds, char* words, size_t length);
