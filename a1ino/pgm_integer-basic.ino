// SPDX-License-Identifier: GPL-2.0
#include "inc/pgm.h"

static const char __integer_basic_name[] PROGMEM = "E000 R - INTEGER BASIC";

static const uint8_t __integer_basic_e000[] PROGMEM = {
#include "rom/pgm_integer-basic_e000.h"
};

#define INTEGER_BASIC_ROM_BASE    0xE000
#define INTEGER_BASIC_ROM_SIZE    sizeof(__integer_basic_e000)

struct a1ino_pgm *__integer_basic_get_instance(void) {
  static struct a1ino_pgm pgm;

  pgm.name = __integer_basic_name;
  pgm.base = INTEGER_BASIC_ROM_BASE;

  e8bit_rom_setup(&pgm.rom,
      INTEGER_BASIC_ROM_BASE, INTEGER_BASIC_ROM_SIZE, __integer_basic_e000);

  return &pgm;
}

struct a1ino_pgm *pgm_integer_basic_get_instance(void) {
  static struct a1ino_pgm *pgm;

  if (pgm)
    return pgm;

  pgm = __integer_basic_get_instance();

  return pgm;
}
