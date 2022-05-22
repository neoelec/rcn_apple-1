// SPDX-License-Identifier: GPL-2.0
#include "inc/pgm.h"

static const char __osi_6502_basic_name[] PROGMEM = "OSI 6502 BASIC";

static const uint8_t __osi_6502_basic_a000[] PROGMEM = {
#include "rom/pgm_osi-6502-basic_a000.h"
};

#define OSI_6502_BASIC_ROM_BASE     0xA000
#define OSI_6502_BASIC_ROM_RUN      0xBD0D
#define OSI_6502_BASIC_ROM_SIZE     sizeof(__osi_6502_basic_a000)

struct a1ino_pgm *__osi_6502_basic_get_instance(void) {
  static struct a1ino_pgm pgm;

  pgm.name = __osi_6502_basic_name;
  pgm.base = OSI_6502_BASIC_ROM_BASE;
  pgm.run = OSI_6502_BASIC_ROM_RUN;

  e8bit_rom_setup(&pgm.rom,
      OSI_6502_BASIC_ROM_BASE, OSI_6502_BASIC_ROM_SIZE, __osi_6502_basic_a000);

  return &pgm;
}

struct a1ino_pgm *pgm_osi_6502_basic_get_instance(void) {
  static struct a1ino_pgm *pgm;

  if (pgm)
    return pgm;

  pgm = __osi_6502_basic_get_instance();

  return pgm;
}
