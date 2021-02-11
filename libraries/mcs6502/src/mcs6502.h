#ifndef __MCS6502_H__
#define __MCS6502_H__

#include <emul_8bit.h>
#include <stdbool.h>
#include <stdint.h>

//#define CFG_MCS6502_USE_TIMING

#define MCS6502_CPU_PAUSE  -1

#define MCS6502_NMI_VECTOR_LOW    0xFFFA
#define MCS6502_NMI_VECTOR_HIGH   0xFFFB

#define MCS6502_RESET_VECTOR_LOW	0xFFFC
#define MCS6502_RESET_VECTOR_HIGH	0xFFFD

#define MCS6502_IRQ_VECTOR_LOW    0xFFFE
#define MCS6502_IRQ_VECTOR_HIGH   0xFFFF

#define MCS6502_STACK_BASE        0x0100

struct mcs6502_register {
  /* registers */
  uint8_t A;                    /* ACCUMULATOR */
  uint8_t Y;                    /* Y INDEX REG */
  uint8_t X;                    /* X INDEX REG */
  union {
    uint16_t PC;                /* PROGRAM COUNTER */
    struct {
      uint8_t PCL;
      uint8_t PCH;
    };
  } __attribute__((packed));
  uint8_t S;                    /* STACK PNTR */
  union {
    uint8_t raw;
    struct {
      uint8_t C:1;              /* carry = _borrow */
      uint8_t Z:1;              /* zero result */
      uint8_t I:1;              /* IRQ disabled */
      uint8_t D:1;              /* decimal mode */
      uint8_t B:1;              /* BRK instruction */
      uint8_t U:1;              /* unused? */
      uint8_t V:1;              /* overflow */
      uint8_t N:1;              /* negative result */
    } __attribute__((packed));
  } P;                          /* FLAGS */
};

enum mcs6502_irq_pin_t {
  E_MCS6502_IRQ_PIN_H = 0,      /* HIGH */
  E_MCS6502_IRQ_PIN_L,          /* LOW */
  E_MCS6502_IRQ_PIN_H_L_H,      /* HIGH -> LOW -> HIGH */
};

enum mcs6502_irq_state_t {
  E_MCS6502_IRQ_STATE_OUT = 0,
  E_MCS6502_IRQ_STATE_IN,
  E_MCS6502_IRQ_STATE_EXIT,
};

enum mcs6502_fetch_src_t {
  E_MCS6502_FETCH_SRC_REG_A,
  E_MCS6502_FETCH_SRC_BUS_ADDR,
};

struct mcs6502 {
  struct mcs6502_register reg;

  enum mcs6502_irq_pin_t nmi;
  enum mcs6502_irq_pin_t irq;
  enum mcs6502_irq_state_t irq_state;

  enum mcs6502_fetch_src_t fetch_src;
  union {
    uint16_t bus_addr;
    struct {
      uint8_t bus_addr_l;
      uint8_t bus_addr_h;
    };
  };

#ifdef CFG_MCS6502_USE_TIMING
  int clk_per_instr, clk_goal;
  unsigned long cnt_instr;
#else
  unsigned long tickcount;
#endif

  struct e8bit_bus bus;
  struct e8bit_hook hook;

  struct e8bit_bus_dev *r_cache;  /* D-Cache - Read */
  struct e8bit_bus_dev *w_cache;  /* D-Cache - Write */
  struct e8bit_bus_dev *i_cache;  /* I-Cache */
  struct e8bit_bus_dev *s_cache;  /* D-Cache - Stack */
};

#ifdef __cplusplus
extern "C" {
#endif

extern void mcs6502_setup(struct mcs6502 *cpu);
extern void mcs6502_reset(struct mcs6502 *cpu);
extern int mcs6502_exec(struct mcs6502 *cpu, unsigned long tickcount);
extern void mcs6502_change_nmi(struct mcs6502 *cpu, enum mcs6502_irq_pin_t state);
extern void mcs6502_change_irq(struct mcs6502 *cpu, enum mcs6502_irq_pin_t state);

#ifdef __cplusplus
}
#endif

#endif /* __MCS6502_H_ */
