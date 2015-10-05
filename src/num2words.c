#include "num2words.h"
#include "strings-en.h"
#include "strings-sv.h"
#include "string.h"

static size_t append_string(char* buffer, const size_t length, const char* str) {
  strncat(buffer, str, length);

  size_t written = strlen(str);
  return (length > written) ? written : length;
}

void time_to_words(Language lang, int hours, int minutes, int seconds, char* words, size_t buffer_size) {

  size_t remaining = buffer_size;
  memset(words, 0, buffer_size);

  // We want to operate with a resolution of 30 seconds.  So multiply
  // minutes and seconds by 2.  Then divide by (2 * 5) to carve the hour
  // into five minute intervals.
  int half_mins  = (2 * minutes) + (seconds / 30);
  int rel_index  = ((half_mins + 5) / (2 * 5)) % 12;
  int hour_index;

  if (rel_index > 6 || (rel_index == 0 && minutes > 30)) {
    hour_index = (hours + 1) % 12;
  }
  else {
    hour_index = hours % 12;
  }

  const char* hour = lang == EN ? HOURS_EN[hour_index] : HOURS_SV[hour_index];
  const char* rel  = lang == EN ? RELS_EN[rel_index]   : RELS_SV[rel_index];

  if (rel_index == 0) {
    // The "*" makes the hour bold.
    remaining -= append_string(words, remaining, "*");
    remaining -= append_string(words, remaining, hour);
    remaining -= append_string(words, remaining, rel);
  }
  else {
    remaining -= append_string(words, remaining, rel);
    remaining -= append_string(words, remaining, "*");
    remaining -= append_string(words, remaining, hour);
  }

  // Leave one space at the end
  remaining -= append_string(words, remaining, " ");

}
