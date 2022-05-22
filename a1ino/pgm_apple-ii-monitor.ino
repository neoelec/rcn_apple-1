// SPDX-License-Identifier: GPL-2.0
#include "inc/pgm.h"

static const char __apple_ii_monitor_name[] PROGMEM = "APPLE-II MONITOR";

static const uint8_t __apple_ii_monitor_f400[] PROGMEM = {
#include "rom/pgm_apple-ii-monitor_f400.h"
};

#define APPLE_II_MONITOR_ROM_BASE   0xF400
#define APPLE_II_MONITOR_ROM_RUN    0xFE59
#define APPLE_II_MONITOR_ROM_SIZE   sizeof(__apple_ii_monitor_f400)

struct a1ino_pgm *__apple_ii_monitor_get_instance(void) {
  static struct a1ino_pgm pgm;

  pgm.name = __apple_ii_monitor_name;
  pgm.base = APPLE_II_MONITOR_ROM_BASE;
  pgm.run = APPLE_II_MONITOR_ROM_RUN;

  e8bit_rom_setup(&pgm.rom,
      APPLE_II_MONITOR_ROM_BASE, APPLE_II_MONITOR_ROM_SIZE, __apple_ii_monitor_f400);

  return &pgm;
}

struct a1ino_pgm *pgm_apple_ii_monitor_get_instance(void) {
  static struct a1ino_pgm *pgm;

  if (pgm)
    return pgm;

  pgm = __apple_ii_monitor_get_instance();

  return pgm;
}
