// SPDX-License-Identifier: GPL-2.0
#include "inc/pgm.h"

static const char __ehbasic_name[] PROGMEM = "Enhanced BASIC 1.10";

static const uint8_t __ehbasic_5800[] PROGMEM = {
#include "rom/pgm_ehbasic_5800.h"
};

#define EHBASIC_ROM_BASE      0x5800
#define EHBASIC_ROM_RUN       EHBASIC_ROM_BASE
#define EHBASIC_ROM_SIZE      sizeof(__ehbasic_5800)

struct a1ino_pgm *__ehbasic_get_instance(void) {
  static struct a1ino_pgm pgm;

  pgm.name = __ehbasic_name;
  pgm.base = EHBASIC_ROM_BASE;
  pgm.run = EHBASIC_ROM_RUN;

  e8bit_rom_setup(&pgm.rom,
      EHBASIC_ROM_BASE, EHBASIC_ROM_SIZE, __ehbasic_5800);

  return &pgm;
}

struct a1ino_pgm *pgm_ehbasic_get_instance(void) {
  static struct a1ino_pgm *pgm;

  if (pgm)
    return pgm;

  pgm = __ehbasic_get_instance();

  return pgm;
}
