// SPDX-License-Identifier: GPL-2.0
#include "inc/pgm.h"

static const char __tiny_basic_name[] PROGMEM = "TINY BASIC";

static const uint8_t __tiny_basic_7600[] PROGMEM = {
#include "rom/pgm_tiny-basic_7600.h"
};

#define TINY_BASIC_ROM_BASE     0x7600
#define TINY_BASIC_ROM_RUN      TINY_BASIC_ROM_BASE
#define TINY_BASIC_ROM_SIZE     sizeof(__tiny_basic_7600)

struct a1ino_pgm *__tiny_basic_get_instance(void) {
  static struct a1ino_pgm pgm;

  pgm.name = __tiny_basic_name;
  pgm.base = TINY_BASIC_ROM_BASE;
  pgm.run = TINY_BASIC_ROM_RUN;

  e8bit_rom_setup(&pgm.rom,
      TINY_BASIC_ROM_BASE, TINY_BASIC_ROM_SIZE, __tiny_basic_7600);

  return &pgm;
}

struct a1ino_pgm *pgm_tiny_basic_get_instance(void) {
  static struct a1ino_pgm *pgm;

  if (pgm)
    return pgm;

  pgm = __tiny_basic_get_instance();

  return pgm;
}
