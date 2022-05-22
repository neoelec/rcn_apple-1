#ifndef __A1INO_PGM_H__
#define __A1INO_PGM_H__

struct a1ino_pgm {
  struct e8bit_rom rom;
  const char *name;
  uint16_t base;
  uint16_t run;
};

#endif /* __A1INO_PGM_H__ */
