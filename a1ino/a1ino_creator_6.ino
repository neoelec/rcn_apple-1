// SPDX-License-Identifier: GPL-2.0
#include "inc/a1ino.h"
#include "inc/pgm.h"

static struct a1ino_pgm **__creator_6_setup_pgm(size_t *nr_pgm) {
  static struct a1ino_pgm *pgm[3];

  pgm[0] = pgm_integer_basic_get_instance();
  pgm[1] = pgm_krusader_1_3_get_instance();
  pgm[2] = pgm_ehbasic_get_instance();

  *nr_pgm = ARRAY_SIZE(pgm);

  return pgm;
}

static struct a1ino_rom *__creator_6(void) {
  static struct a1ino_pgm **pgm;
  static size_t nr_pgm;

  if (!pgm)
    pgm = __creator_6_setup_pgm(&nr_pgm);

  return a1ino_rom_get_instance(pgm, nr_pgm);
}

void a1ino_create_6(struct a1ino *emul) {
  a1ino_creator_template(emul, __creator_6);
}

void a1ino_describe_6(size_t idx) {
  a1ino_describe_template(idx, __creator_6);
}
