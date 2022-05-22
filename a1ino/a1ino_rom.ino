// SPDX-License-Identifier: GPL-2.0
#include "inc/a1ino.h"

struct a1ino_rom *a1ino_rom_get_instance(struct a1ino_pgm **pgm, size_t nr_pgm) {
  static struct a1ino_rom rom;

  rom.pgm = pgm;
  rom.nr_pgm = nr_pgm;

  return &rom;
}

void a1ino_rom_install(struct a1ino *emul) {
  struct a1ino_rom *rom = emul->rom;
  struct mcs6502 *cpu = a1ino_get_cpu(emul);
  size_t i;

  for (i = 0; i < rom->nr_pgm; i++) {
    struct a1ino_pgm *pgm = rom->pgm[i];

    e8bit_bus_attach(&cpu->bus, &pgm->rom.dev);
  }
}

void a1ino_rom_uninstall(struct a1ino *emul) {
  struct a1ino_rom *rom = emul->rom;
  struct mcs6502 *cpu = a1ino_get_cpu(emul);
  size_t i;

  for (i = 0; i < rom->nr_pgm; i++) {
    struct a1ino_pgm *pgm = rom->pgm[i];

    e8bit_bus_detach(&cpu->bus, &pgm->rom.dev);
  }
}
