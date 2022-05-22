// SPDX-License-Identifier: GPL-2.0
#include "inc/a1ino.h"

extern void a1ino_creator_1(struct a1ino *emul);
extern void a1ino_creator_2(struct a1ino *emul);
extern void a1ino_creator_3(struct a1ino *emul);
extern void a1ino_creator_4(struct a1ino *emul);
extern void a1ino_creator_5(struct a1ino *emul);

typedef void (*creator_t)(struct a1ino *);

static creator_t creator[] = {
  a1ino_creator_1,              /* fall back */
  a1ino_creator_1,
  a1ino_creator_2,
  a1ino_creator_3,
  a1ino_creator_4,
  a1ino_creator_5,
};

struct a1ino *a1ino_factory(size_t prod_id) {
  static struct a1ino __emul;
  struct a1ino *emul = &__emul;

  if (prod_id >= ARRAY_SIZE(creator))
    prod_id = 0;                /* fall back */

  creator[prod_id](emul);

  a1ino_tty_install(emul);
  a1ino_ram_install(emul);
  a1ino_rom_install(emul);

  return emul;
}

void a1ino_creator_template(struct a1ino *emul, void (*concrete_creator)(struct a1ino *)) {
  uint16_t rom_base = 0xFFFF;
  uint16_t ram_max;
  size_t i;

  emul->board = a1ino_board_get_instance();
  emul->tty = a1ino_tty_get_instance();

  if (concrete_creator)
    concrete_creator(emul);

  A1INO_Serial.println(F("* ROM :"));
  for (i = 0; i < emul->rom->nr_pgm; i++) {
    struct a1ino_pgm *pgm = emul->rom->pgm[i];
    static const char run_fmt[] PROGMEM = "%04X R - ";

    A1INO_Serial.print(F("  - "));
    a1ino_printf_P(run_fmt, pgm->run);
    a1ino_putstrln_P(pgm->name);
    rom_base = min(rom_base, pgm->base);
  }

  ram_max = rom_base - APPLE_1_RAM_0000_SIZE;

  emul->ram = a1ino_ram_get_instance(ram_max);

  A1INO_Serial.print(F("* RAM : "));
  A1INO_Serial.print((uint16_t)(APPLE_1_RAM_0000_SIZE + emul->ram->size));
  A1INO_Serial.println(F(" bytes"));
}

size_t a1ino_get_nr_prod(void) {
  return ARRAY_SIZE(creator) - 1;
}
