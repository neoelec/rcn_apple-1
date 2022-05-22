// SPDX-License-Identifier: GPL-2.0
#include "inc/a1ino.h"

extern void a1ino_create_1(struct a1ino *emul);
extern void a1ino_describe_1(size_t idx);

extern void a1ino_create_2(struct a1ino *emul);
extern void a1ino_describe_2(size_t idx);

extern void a1ino_create_3(struct a1ino *emul);
extern void a1ino_describe_3(size_t idx);

extern void a1ino_create_4(struct a1ino *emul);
extern void a1ino_describe_4(size_t idx);

extern void a1ino_create_5(struct a1ino *emul);
extern void a1ino_describe_5(size_t idx);

extern void a1ino_create_6(struct a1ino *emul);
extern void a1ino_describe_6(size_t idx);

#define ROM_DATA(__create, __describe) \
  { .create = __create, .describe = __describe, }

struct {
  void (*create)(struct a1ino *emul);
  void (*describe)(size_t idx);
} rom_data[] = {
  ROM_DATA(a1ino_create_1, a1ino_describe_1), /* fall back */
  ROM_DATA(a1ino_create_1, a1ino_describe_1),
  ROM_DATA(a1ino_create_2, a1ino_describe_2),
  ROM_DATA(a1ino_create_3, a1ino_describe_3),
  ROM_DATA(a1ino_create_4, a1ino_describe_4),
  ROM_DATA(a1ino_create_5, a1ino_describe_5),
  ROM_DATA(a1ino_create_6, a1ino_describe_6),
};

struct a1ino *a1ino_factory(size_t prod_id) {
  static struct a1ino __emul;
  struct a1ino *emul = &__emul;

  if (prod_id >= ARRAY_SIZE(rom_data))
    prod_id = 0;                /* fall back */

  rom_data[prod_id].create(emul);

  a1ino_tty_install(emul);
  a1ino_ram_install(emul);
  a1ino_rom_install(emul);

  return emul;
}

void a1ino_print_supported_product(void) {
  size_t prod_id;

 A1INO_Serial.println(F("* Supported Device Types :"));

  for (prod_id = 1; prod_id < ARRAY_SIZE(rom_data); prod_id++)
    rom_data[prod_id].describe(prod_id);

  A1INO_Serial.println();
}

void a1ino_creator_template(struct a1ino *emul, a1ino_rom *(*concrete_creator)(void)) {
  uint16_t rom_base = 0xFFFF;
  uint16_t ram_max;
  size_t i;

  emul->board = a1ino_board_get_instance();
  emul->tty = a1ino_tty_get_instance();

  if (concrete_creator)
    emul->rom = concrete_creator();

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
  return ARRAY_SIZE(rom_data) - 1;
}

void a1ino_describe_template(size_t idx, a1ino_rom *(*concrete_creator)(void)) {
  a1ino_rom *rom = concrete_creator();
  static const char idx_fmt[] PROGMEM = "[%3zu] - ";
  size_t i;

  a1ino_printf_P(idx_fmt, idx);

  if (rom->nr_pgm == 0)
    goto __exit;

  a1ino_putstr_P(rom->pgm[0]->name);

  for (i = 1; i < rom->nr_pgm; i++) {
    struct a1ino_pgm *pgm = rom->pgm[i];

    A1INO_Serial.print(F(" + "));
    a1ino_putstr_P(pgm->name);
  }

__exit:
  A1INO_Serial.println();
}
