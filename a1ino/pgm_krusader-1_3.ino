// SPDX-License-Identifier: GPL-2.0
#include "inc/pgm.h"

static const char __krusader_1_3_name[] PROGMEM = "F000 R - KRUSADER 1.3";

static const uint8_t __krusader_1_3_f000[] PROGMEM = {
#include "rom/pgm_krusader-1_3_f000.h"
};

#define KRUSADER_1_3_ROM_BASE     0xF000
#define KRUSADER_1_3_ROM_SIZE     sizeof(__krusader_1_3_f000)

struct a1ino_pgm *__krusader_1_3_get_instance(void) {
  static struct a1ino_pgm pgm;

  pgm.name = __krusader_1_3_name;
  pgm.base = KRUSADER_1_3_ROM_BASE;

  e8bit_rom_setup(&pgm.rom,
      KRUSADER_1_3_ROM_BASE, KRUSADER_1_3_ROM_SIZE, __krusader_1_3_f000);

  return &pgm;
}

struct a1ino_pgm *pgm_krusader_1_3_get_instance(void) {
  static struct a1ino_pgm *pgm;

  if (pgm)
    return pgm;

  pgm = __krusader_1_3_get_instance();

  return pgm;
}
