#include "num2words-en.h"
#include "string.h"
//#include <stdio.h>

static const char* const ONES[] = {
  "cero",
  "una",
  "dos",
  "tres",
  "cuatro",
  "cinco",
  "seis",
  "siete",
  "ocho",
  "nueve"
};

static const char* const TEENS[] ={
  "diez",
  "once",
  "doce",
  "trece",
  "catorce",
  "quince",
  "dieciséis",
  "diecisiete",
  "dieciocho",
  "diecinueve"
};

static const char* const TENS[] = {
  "",
  "diez",
  "veinte",
  "treinta",
  "cuarenta",
  "cincuenta",
  "sesenta",
  "setenta",
  "ochenta",
  "noventa"
};

#define QUARTER "cuarto"
#define HALF "media"

#define PAST "y"
#define TO "menos"
#define OCLOCKCONNECTOR "en"
#define OCLOCK "punto"

static const char* const TENSPLUS[] = {
  "",
  "dieci",
  "veinti",
  "treinta y ",
  "cuarenta y ",
  "cincuenta y ",
  "sesenta y ",
  "setenta y ",
  "ochenta y ",
  "noventa y "
};


#define MAX_LEN 7

static size_t append_number(char* words, int num, bool plus = false) {
  int tens_val = num / 10 % 10;
  int ones_val = num % 10;

  size_t len = 0;

  if (tens_val > 0) {
    if (tens_val == 1 ) {
      strcat(words, TEENS[ones_val]);
      return strlen(TEENS[ones_val]);
    }
    if (!plus) {
      strcat(words, TENS[tens_val]);
      len += strlen(TENS[tens_val]);
    } else {
      strcat(words, TENSPLUS[tens_val]);
      len += strlen(TENSPLUS[tens_val]);
    }
  }

  if (ones_val > 0 || num == 0) {
    strcat(words, ONES[ones_val]);
    len += strlen(ONES[ones_val]);
  }

  if (len > MAX_LEN)
  {
    int fullSize = strlen(words);
    words[fullSize - (len - MAX_LEN)] = 0;
    len = MAX_LEN;
  }

  return len;
}

static size_t append_string(char* buffer, const size_t length, const char* str) {
  strncat(buffer, str, length);

  size_t written = strlen(str);
  return (length > written) ? written : length;
}


void time_to_words(int hours, int minutes, char* words, size_t length) {

  size_t remaining = length;
  memset(words, 0, length);
  // Fuzzy time
  minutes = (minutes + 3) / 5 * 5;
  // Handle hour wrapping
  if (minutes > 30) hours++;
  hours += 12; // If hours == 0
  while (hours > 12)
  {
    hours -= 12;
  }

  remaining -= append_string(words, remaining, "*"); // Make hours bold
  remaining -= append_number(words, hours);
  remaining -= append_string(words, remaining, " ");

  // Add o'clock to whole hours
  if (minutes == 0)
  {
    remaining -= append_string(words, remaining, OCLOCKCONNECTOR);
    remaining -= append_string(words, remaining, " ");    
    remaining -= append_string(words, remaining, OCLOCK);
  }

  

  // Handle minute wrapping
  if (minutes > 55)
  {
    minutes -= 60;
    hours++;
  }

  switch (minutes)
  {
    case 0:
      break;
    case 5:
    case 10:
    case 20:
      remaining -= append_string(words, remaining, PAST);
      remaining -= append_string(words, remaining, " ");
      remaining -= append_number(words, minutes);
      remaining -= append_string(words, remaining, " ");
      break;
    case 15:
      remaining -= append_string(words, remaining, PAST);
      remaining -= append_string(words, remaining, " ");
      remaining -= append_string(words, remaining, QUARTER);
      remaining -= append_string(words, remaining, " ");
      break;
    case 25:
      remaining -= append_string(words, remaining, PAST);
      remaining -= append_string(words, remaining, " ");    
      remaining -= append_number(words, 20, true);
      remaining -= append_string(words, remaining, " ");
      remaining -= append_number(words, 5);
      remaining -= append_string(words, remaining, " ");    
      break;
    case 30:
      remaining -= append_string(words, remaining, PAST);
      remaining -= append_string(words, remaining, " ");
      remaining -= append_string(words, remaining, HALF);
      remaining -= append_string(words, remaining, " ");
      break;
    case 35:
      remaining -= append_string(words, remaining, TO);
      remaining -= append_string(words, remaining, " ");    
      remaining -= append_number(words, 25, true);
      remaining -= append_string(words, remaining, " ");
      remaining -= append_number(words, 5);
      remaining -= append_string(words, remaining, " ");    
      break;
    case 40:
    case 50:
    case 55:
      remaining -= append_string(words, remaining, TO);
      remaining -= append_string(words, remaining, " ");
      remaining -= append_number(words, 60 - minutes);
      remaining -= append_string(words, remaining, " ");
      break;
    case 45:
      remaining -= append_string(words, remaining, TO);
      remaining -= append_string(words, remaining, " ");
      remaining -= append_string(words, remaining, QUARTER);
      remaining -= append_string(words, remaining, " ");
      break;
  }

}

