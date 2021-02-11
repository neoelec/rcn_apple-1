#ifndef __MC6820_H__
#define __MC6820_H__

#include <emul_8bit.h>

struct mc6820_register {
  uint8_t ora;
  union {
    uint8_t cra;
    struct {
      uint8_t ca1_ctrl:2;
      uint8_t ddra:1;
      uint8_t ca2_ctrl:3;
      uint8_t irqa2:1;
      uint8_t irqa1:1;
    };
  };
  uint8_t orb;
  union {
    uint8_t crb;
    struct {
      uint8_t cb1_ctrl:2;
      uint8_t ddrb:1;
      uint8_t cb2_ctrl:3;
      uint8_t irqb2:1;
      uint8_t irqb1:1;
    };
  };
} __attribute__((packed));

#define MC6820_IO_OFFSET_ORA  (offsetof(struct mc6820_register, ora))
#define MC6820_IO_OFFSET_CRA  (offsetof(struct mc6820_register, cra))
#define MC6820_IO_OFFSET_ORB  (offsetof(struct mc6820_register, orb))
#define MC6820_IO_OFFSET_CRB  (offsetof(struct mc6820_register, crb))

#define MC6820_IO_SIZE  sizeof(struct mc6820_register)

struct mc6820 {
  struct e8bit_io io;
  union {
    struct mc6820_register reg;
    uint8_t io_area[MC6820_IO_SIZE];
  };
};

#ifdef __cplusplus
extern "C" {
#endif

extern void mc6820_setup(struct mc6820 *pia, uint16_t io_base);
extern void mc6820_install(struct mc6820 *pia, struct e8bit_bus *bus);
extern void mc6820_uninstall(struct mc6820 *pia, struct e8bit_bus *bus);

#ifdef __cplusplus
}
#endif

#endif /* __MC6820_H__ */
