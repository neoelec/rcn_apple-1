// SPDX-License-Identifier: GPL-2.0
#include "inc/a1ino.h"

void a1ino_puts_P(const char *str) {
  size_t i;

  for (i = 0; i < strlen_P(str); i++) {
    char c = pgm_read_byte_near(str + i);

    A1INO_Serial.print(c);
  }

  A1INO_Serial.println();
}

#ifndef PRINTF_BUF_SZ
#define PRINTF_BUF_SZ   128
#endif

void a1ino_printf_P(const char *fmt, ...) {
  char buf[PRINTF_BUF_SZ];      // resulting string limited to 128 chars
  char *__buf = buf;
  va_list args;

  va_start(args, fmt);
#ifdef __AVR__
  vsnprintf_P(buf, sizeof(buf), (const char *)fmt, args); // progmem for AVR
#else
  vsnprintf(buf, sizeof(buf), (const char *)fmt, args); // for the rest of the world
#endif
  va_end(args);

  while (*__buf) {
    A1INO_Serial.write(*__buf);
    if (*__buf == '\n')
      A1INO_Serial.write('\r');

    __buf++;
  }
}
