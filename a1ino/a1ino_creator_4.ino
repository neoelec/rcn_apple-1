// SPDX-License-Identifier: GPL-2.0
#include "inc/a1ino.h"
#include "inc/pgm.h"

static struct a1ino_pgm **__creator_4_setup_pgm(size_t *nr_pgm) {
  static struct a1ino_pgm *pgm[3];

  pgm[0] = pgm_integer_basic_get_instance();
  pgm[1] = pgm_krusader_1_3_get_instance();
  pgm[2] = pgm_osi_6502_basic_get_instance();

  *nr_pgm = ARRAY_SIZE(pgm);

  return pgm;
}

static void __creator_4(struct a1ino *emul) {
  static struct a1ino_pgm **pgm;
  static size_t nr_pgm;

  if (!pgm)
    pgm = __creator_4_setup_pgm(&nr_pgm);

  emul->rom = a1ino_rom_get_instance(pgm, nr_pgm);
}

void a1ino_creator_4(struct a1ino *emul) {
  a1ino_creator_template(emul, __creator_4);
}
