#ifndef __A1INO_RAM_H__
#define __A1INO_RAM_H__

#include <emul_8bit.h>

#ifdef CFG_A1INO_RAM
#define A1INO_RAM_1000_SIZE   CFG_A1INO_RAM_1000_SIZE
#else
#define A1INO_RAM_1000_SIZE   0x0000
#endif

struct a1ino_ram {
  struct e8bit_ram ram_1000;
  uint8_t mem[A1INO_RAM_1000_SIZE];
  uint16_t size;
};

#if defined(CFG_A1INO_RAM) && (A1INO_RAM_1000_SIZE > 0x0)
extern struct a1ino_ram *a1ino_ram_get_instance(uint16_t size);

extern void a1ino_ram_install(struct a1ino *emul);
extern void a1ino_ram_uninstall(struct a1ino *emul);
#else
static inline struct a1ino_ram *a1ino_ram_get_instance(uint16_t size) { return NULL; }

static inline void a1ino_ram_install(struct a1ino *emul) {}
static inline void a1ino_ram_uninstall(struct a1ino *emul) {}
#endif

#endif /* __A1INO_RAM_H__ */
