// SPDX-License-Identifier: GPL-2.0
#include <stdlib.h>

#include "mcs6502.h"

#pragma GCC optimize ("O3")

static inline uint8_t __mcs6502_bus_read(struct mcs6502 *cpu, uint16_t addr) {
  uint8_t val;
  struct e8bit_bus_dev *r_cache;

  if (cpu->r_cache) {
    if (e8bit_bus_read_dev(cpu->r_cache, addr, &val))
      return val;
  }

  r_cache = e8bit_bus_read(&cpu->bus, addr, &val);
  if (r_cache)
    cpu->r_cache = r_cache;

  return val;
}

static inline void __mcs6502_bus_write(struct mcs6502 *cpu,
    uint16_t addr, uint8_t val) {
  struct e8bit_bus_dev *w_cache;

  if (cpu->w_cache) {
    if (e8bit_bus_write_dev(cpu->w_cache, addr, val))
      return;
  }

  w_cache = e8bit_bus_write(&cpu->bus, addr, val);
  if (w_cache)
    cpu->w_cache = w_cache;
}

static inline uint8_t __mcs6502_fetch_pc(struct mcs6502 *cpu) {
  uint8_t val;
  struct e8bit_bus_dev *i_cache;
  uint16_t PC = cpu->reg.PC++;

  if (cpu->i_cache) {
    if (e8bit_bus_read_dev(cpu->i_cache, PC, &val))
      return val;
  }

  i_cache = e8bit_bus_read(&cpu->bus, PC, &val);
  if (i_cache)
    cpu->i_cache = i_cache;

  return val;
}

void mcs6502_setup(struct mcs6502 *cpu) {
  e8bit_bus_setup(&cpu->bus);
  e8bit_hook_setup(&cpu->hook);

  cpu->r_cache = NULL;
  cpu->w_cache = NULL;
  cpu->i_cache = NULL;
  cpu->s_cache = NULL;
}

void mcs6502_reset(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  reg->A = 0;
  reg->X = 0;
  reg->Y = 0;
  reg->S = 0xFD;
  reg->P.U = 1;
  reg->PCL = __mcs6502_bus_read(cpu, MCS6502_RESET_VECTOR_LOW);
  reg->PCH = __mcs6502_bus_read(cpu, MCS6502_RESET_VECTOR_HIGH);

  cpu->irq_state = E_MCS6502_IRQ_STATE_IN;
}

// slower, but allows you to specify number of cycles to run for exec6502
// rather than simply a number of instructions. also uses a little more
// program memory when enabled.
static inline void __mcs6502_set_clk_per_instr(struct mcs6502 *cpu,
    int n) {
#ifdef CFG_MCS6502_USE_TIMING
  cpu->clk_per_instr = n;
#else
  cpu = cpu;
  n = n;
#endif
}

static inline void __mcs6502_increase_clk_per_instr(struct mcs6502 *cpu,
    int n) {
#ifdef CFG_MCS6502_USE_TIMING
  cpu->clk_per_instr += n;
#else
  cpu = cpu;
  n = n;
#endif
}

static inline void __mcs6502_push_byte(struct mcs6502 *cpu, uint8_t val) {
  struct mcs6502_register *reg = &cpu->reg;
  struct e8bit_bus_dev *s_cache;
  uint16_t reg_S = MCS6502_STACK_BASE + reg->S--;

  if (cpu->s_cache) {
    if (e8bit_bus_write_dev(cpu->s_cache, reg_S, val))
      return;
  }

  s_cache = e8bit_bus_write(&cpu->bus, reg_S, val);
  if (s_cache)
    cpu->s_cache = s_cache;
}

static inline void __mcs6502_push_word(struct mcs6502 *cpu, uint16_t val) {
  union {
    uint16_t w;
    uint8_t b[2];
  } _val = {
    .w = val,
  };

  __mcs6502_push_byte(cpu, _val.b[1]);
  __mcs6502_push_byte(cpu, _val.b[0]);
}

static inline uint8_t __mcs6502_pop_byte(struct mcs6502 *cpu) {
  uint8_t val;
  struct mcs6502_register *reg = &cpu->reg;
  struct e8bit_bus_dev *s_cache;
  uint16_t reg_S = MCS6502_STACK_BASE + ++reg->S;

  if (cpu->s_cache) {
    if (e8bit_bus_read_dev(cpu->s_cache, reg_S, &val))
      return val;
  }

  s_cache = e8bit_bus_read(&cpu->bus, reg_S, &val);
  if (s_cache)
    cpu->s_cache = s_cache;

  return val;
}

static inline uint16_t __mcs6502_pop_word(struct mcs6502 *cpu) {
  union {
    uint16_t w;
    uint8_t b[2];
  } val;

  val.b[0] = __mcs6502_pop_byte(cpu);
  val.b[1] = __mcs6502_pop_byte(cpu);

  return val.w;
}

static inline uint8_t __mcs6502_from_bcd(uint8_t bcd) {
  return ((bcd >> 4) & 0x0F) * 10 + (bcd & 0x0F);
}

static inline uint8_t __mcs6502_to_bcd(uint8_t hex) {
  return (((hex % 100) / 10) << 4) | (hex % 10);
}

static inline void __mcs6502_update_P_Z(struct mcs6502_register *reg,
    uint8_t val) {
  reg->P.Z = val ? 0 : 1;
}

static inline void __mcs6502_update_P_N(struct mcs6502_register *reg,
    uint8_t val) {
  reg->P.N = val & 0x80 ? 1 : 0;
}

static inline void __mcs6502_update_P_C(struct mcs6502_register *reg,
    uint16_t val) {
  reg->P.C = val & 0xFF00 ? 1 : 0;
}

static inline void __mcs6502_update_P_V(struct mcs6502_register *reg,
    uint16_t val, uint8_t in_0, uint8_t in_1) {
  reg->P.V = ((val ^ (uint16_t)in_0) & (val ^ (uint16_t)in_1) & 0x0080) ? 1 : 0;
}

/* core addressing */
/*  : None */
static inline void __mcs6502_access_none(struct mcs6502 *cpu) {
  __mcs6502_set_clk_per_instr(cpu, 1);
}

/* #n : Immediate */
static inline void __mcs6502_access_immediate(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  __mcs6502_set_clk_per_instr(cpu, 2);

  cpu->bus_addr = reg->PC++;
}

/* n : Zero Page */
static inline void __mcs6502_access_zero_page(struct mcs6502 *cpu) {
  __mcs6502_set_clk_per_instr(cpu, 2);

  cpu->bus_addr = (uint16_t)__mcs6502_fetch_pc(cpu);
}

/* nn : Absolute */
static inline void __mcs6502_access_absolute(struct mcs6502 *cpu) {
  __mcs6502_set_clk_per_instr(cpu, 3);

  cpu->bus_addr_l = __mcs6502_fetch_pc(cpu);
  cpu->bus_addr_h = __mcs6502_fetch_pc(cpu);
}

/* (n),Y : Ind Y */
static inline void __mcs6502_access_indirect_y(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;
  uint16_t in_addr;
  uint16_t start_page;

  __mcs6502_access_zero_page(cpu);
  // __mcs6502_set_clk_per_instr(cpu, 2);

  in_addr = cpu->bus_addr;
  cpu->bus_addr_l = __mcs6502_bus_read(cpu, in_addr);

  in_addr = (in_addr + 1) & 0x00FF;
  cpu->bus_addr_h = __mcs6502_bus_read(cpu, in_addr);

  start_page = cpu->bus_addr & 0xFF00;
  cpu->bus_addr += (uint16_t)reg->Y;
  __mcs6502_increase_clk_per_instr(cpu, start_page != (cpu->bus_addr & 0xFF00));
}

/* (n,X) : Ind X */
static inline void __mcs6502_access_indirect_x(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;
  uint16_t in_addr;

  __mcs6502_access_zero_page(cpu);
  // __mcs6502_set_clk_per_instr(cpu, 2);

  in_addr = cpu->bus_addr + (uint16_t)reg->X;
  in_addr &= 0x00FF;

  cpu->bus_addr_l = __mcs6502_bus_read(cpu, in_addr);
  in_addr = (in_addr + 1) & 0x00FF;
  cpu->bus_addr_h = __mcs6502_bus_read(cpu, in_addr);
}

/* (nn) : Indirect */
static inline void __mcs6502_access_indirect(struct mcs6502 *cpu) {
  uint16_t in_addr;

  __mcs6502_access_absolute(cpu);
  // __mcs6502_set_clk_per_instr(cpu, 3);

  in_addr = cpu->bus_addr;
  cpu->bus_addr_l = __mcs6502_bus_read(cpu, in_addr);

  in_addr = (in_addr & 0xFF00) | ((in_addr + 1) & 0x00FF);
  cpu->bus_addr_h = __mcs6502_bus_read(cpu, in_addr);
}

/* A : Accumulator */
static inline void __mcs6502_access_accumulator(struct mcs6502 *cpu) {
  __mcs6502_set_clk_per_instr(cpu, 1);

  cpu->fetch_src = E_MCS6502_FETCH_SRC_REG_A;
}

/* n,X : Zero Page X */
static inline void __mcs6502_access_zero_page_x(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;
  __mcs6502_access_zero_page(cpu);
  // __mcs6502_set_clk_per_instr(cpu, 2);

  cpu->bus_addr += (uint16_t)reg->X;
  cpu->bus_addr &= 0x00FF;
}

/* n,Y : Zero Page Y */
static inline void __mcs6502_access_zero_page_y(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;
  __mcs6502_access_zero_page(cpu);
  // __mcs6502_set_clk_per_instr(cpu, 2);

  cpu->bus_addr += (uint16_t)reg->Y;
  cpu->bus_addr &= 0x00FF;
}

/* nn,X : Absolute X */
static inline void __mcs6502_access_absolute_x(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;
  uint16_t start_page;

  __mcs6502_access_absolute(cpu);
  // __mcs6502_set_clk_per_instr(cpu, 3);

  start_page = cpu->bus_addr & 0xFF00;
  cpu->bus_addr += (uint16_t)reg->X;
  __mcs6502_increase_clk_per_instr(cpu, start_page != (cpu->bus_addr & 0xFF00));
}

/* nn,Y : Absolute Y */
static inline void __mcs6502_access_absolute_y(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;
  uint16_t start_page;

  __mcs6502_access_absolute(cpu);
  // __mcs6502_set_clk_per_instr(cpu, 3);

  start_page = cpu->bus_addr & 0xFF00;
  cpu->bus_addr += (uint16_t)reg->Y;
  __mcs6502_increase_clk_per_instr(cpu, start_page != (cpu->bus_addr & 0xFF00));
}

/* r : Relative */
static inline void __mcs6502_access_relative(struct mcs6502 *cpu) {
  __mcs6502_set_clk_per_instr(cpu, 2);

  __mcs6502_access_zero_page(cpu);

  if (cpu->bus_addr & 0x0080)
    cpu->bus_addr |= 0xFF00;

  cpu->bus_addr += cpu->reg.PC;
}

static inline uint8_t __mcs6502_fetch_data(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  if (cpu->fetch_src == E_MCS6502_FETCH_SRC_REG_A)
    return reg->A;
  else
    return __mcs6502_bus_read(cpu, cpu->bus_addr);
}

static inline void __mcs6502_write_bus_back_data(struct mcs6502 *cpu,
    uint8_t val) {
  struct mcs6502_register *reg = &cpu->reg;

  if (cpu->fetch_src == E_MCS6502_FETCH_SRC_REG_A)
    reg->A = val;
  else
    __mcs6502_bus_write(cpu, cpu->bus_addr, val);
}

/* core operations */

/* ADC : Add with carry to A */
static inline void __mcs6502_execute_adc(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;
  uint8_t in_b = __mcs6502_fetch_data(cpu);

  if (!reg->P.D) {
    union {
      uint16_t w;
      uint8_t b[2];
    } out;

    out.w = (uint16_t)reg->A + (uint16_t)in_b + (uint16_t)(reg->P.C);

    __mcs6502_update_P_N(reg, out.b[0]);
    __mcs6502_update_P_V(reg, out.w, reg->A, in_b);
    __mcs6502_update_P_Z(reg, out.b[0]);
    __mcs6502_update_P_C(reg, out.w);

    reg->A = out.b[0];
  } else {
    uint8_t out;

    out = __mcs6502_from_bcd(reg->A) - __mcs6502_from_bcd(in_b) + reg->P.C;

    reg->P.C = !!(out > 99);
    if (reg->P.C)
      out -= 100;

    reg->A = __mcs6502_to_bcd(out);

    __mcs6502_update_P_N(reg, out);
    __mcs6502_update_P_Z(reg, out);

    __mcs6502_increase_clk_per_instr(cpu, 1);
  }
}

/* AND : AND to A */
static inline void __mcs6502_execute_and(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  reg->A &= __mcs6502_fetch_data(cpu);

  __mcs6502_update_P_N(reg, reg->A);
  __mcs6502_update_P_Z(reg, reg->A);
}

/* ASL : Arithmetic shift left */
static inline void __mcs6502_execute_asl(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;
  union {
    uint16_t w;
    uint8_t b[2];
  } in = { .w = 0, };

  in.b[0] = __mcs6502_fetch_data(cpu);
  in.w = in.w << 1;

  __mcs6502_update_P_C(reg, in.w);
  __mcs6502_update_P_Z(reg, in.b[0]);
  __mcs6502_update_P_N(reg, in.b[0]);

  __mcs6502_write_bus_back_data(cpu, in.b[0]);
}

/* for BCC / BCS / BEQ / BNE / BMI / BPL / BVC / BVS */
static inline void __mcs6502_branch_common(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;
  uint8_t old_reg_PC = reg->PC;

  reg->PC = cpu->bus_addr;
  if ((old_reg_PC ^ reg->PC) & 0xFF00)
    //check if jump crossed reg->A page boundary
    __mcs6502_increase_clk_per_instr(cpu, 2);
  else
    __mcs6502_increase_clk_per_instr(cpu, 1);
}

/* BCC : Branch if Carry clear (C=0) */
static inline void __mcs6502_execute_bcc(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  if (!reg->P.C)
    __mcs6502_branch_common(cpu);
}

/* BCS : Branch if Carry set (C=1) */
static inline void __mcs6502_execute_bcs(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  if (reg->P.C)
    __mcs6502_branch_common(cpu);
}

/* BEQ : Branch if equal (Z=1) */
static inline void __mcs6502_execute_beq(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  if (reg->P.Z)
    __mcs6502_branch_common(cpu);
}

/* BMI : Branch if minus (N=1) */
static inline void __mcs6502_execute_bmi(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  if (reg->P.N)
    __mcs6502_branch_common(cpu);
}

/* BNE : Branch if not equal (Z=0) */
static inline void __mcs6502_execute_bne(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  if (!reg->P.Z)
    __mcs6502_branch_common(cpu);
}

/* BPL : Branch if plus (N=0) */
static inline void __mcs6502_execute_bpl(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  if (!reg->P.N)
    __mcs6502_branch_common(cpu);
}

/* BVC : Branch if ovfl clear (V=0) */
static inline void __mcs6502_execute_bvc(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  if (!reg->P.V)
    __mcs6502_branch_common(cpu);
}

/* BVS : Branch if ovf l set (V=1) */
static inline void __mcs6502_execute_bvs(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  if (reg->P.V)
    __mcs6502_branch_common(cpu);
}

/* BIT : AND with A (A unchanged) */
static inline void __mcs6502_execute_bit(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;
  uint8_t in_b = __mcs6502_fetch_data(cpu);

  reg->P.raw = (reg->P.raw & 0x3F) | (in_b & 0xC0);

  in_b &= reg->A;

  __mcs6502_update_P_Z(reg, in_b);
}

/* BRK : Break (force interrupt) */
static inline void __mcs6502_execute_brk(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  reg->PC++;
  //push next instruction address onto stack
  __mcs6502_push_word(cpu, reg->PC);
  reg->P.B = 1;
  __mcs6502_push_byte(cpu, reg->P.raw); //push CPU reg->P.raw to stack
  reg->P.B = 0;
  reg->P.I = 1;                 //set interrupt flag

  reg->PCL = __mcs6502_bus_read(cpu, MCS6502_IRQ_VECTOR_LOW);
  reg->PCH = __mcs6502_bus_read(cpu, MCS6502_IRQ_VECTOR_HIGH);

  cpu->irq_state = E_MCS6502_IRQ_STATE_IN;
}

/* CLC : Clear carry */
static inline void __mcs6502_execute_clc(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  reg->P.C = 0;
}

/* CLD : Clear decimal mode */
static inline void __mcs6502_execute_cld(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  reg->P.D = 0;
}

/* CLI : Clear IRQ disable */
static inline void __mcs6502_execute_cli(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  reg->P.I = 0;
}

/* CLV : Clear overflow */
static inline void __mcs6502_execute_clv(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  reg->P.V = 0;
}

/* for CMP / CPX / CPY */
static inline void __mcs6502_compare_common(struct mcs6502 *cpu, uint8_t *val) {
  struct mcs6502_register *reg = &cpu->reg;
  uint8_t in_b = __mcs6502_fetch_data(cpu);

  reg->P.C = *val >= in_b ? 1 : 0;
  reg->P.Z = *val == in_b ? 1 : 0;

  in_b = *val - in_b;

  __mcs6502_update_P_N(reg, in_b);
}

/* CMP : Compare with A */
static inline void __mcs6502_execute_cmp(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  __mcs6502_compare_common(cpu, &reg->A);
}

/* CPX : Compare with X */
static inline void __mcs6502_execute_cpx(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  __mcs6502_compare_common(cpu, &reg->X);
}

/* CPY : Compare with Y */
static inline void __mcs6502_execute_cpy(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  __mcs6502_compare_common(cpu, &reg->Y);
}

/* DEC / DEX / DEY */
static inline void __mcs6502_decrease_common(struct mcs6502 *cpu,
    uint8_t *val) {
  struct mcs6502_register *reg = &cpu->reg;

  (*val)--;

  __mcs6502_update_P_Z(reg, *val);
  __mcs6502_update_P_N(reg, *val);
}

/* DEC : Decrement by one */
static inline void __mcs6502_execute_dec(struct mcs6502 *cpu) {
  uint8_t in_b = __mcs6502_fetch_data(cpu);

  __mcs6502_decrease_common(cpu, &in_b);

  __mcs6502_write_bus_back_data(cpu, in_b);
}

/* DEX : Decrement X by one */
static inline void __mcs6502_execute_dex(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  __mcs6502_decrease_common(cpu, &reg->X);
}

/* DEY : Decrement Y by one */
static inline void __mcs6502_execute_dey(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  __mcs6502_decrease_common(cpu, &reg->Y);
}

/* EOR : XOR to A */
static inline void __mcs6502_execute_eor(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  reg->A ^= __mcs6502_fetch_data(cpu);

  __mcs6502_update_P_Z(reg, reg->A);
  __mcs6502_update_P_N(reg, reg->A);
}

/* INC / INX / INY */
static inline void __mcs6502_increase_common(struct mcs6502 *cpu,
    uint8_t *val) {
  struct mcs6502_register *reg = &cpu->reg;

  (*val)++;

  __mcs6502_update_P_Z(reg, *val);
  __mcs6502_update_P_N(reg, *val);
}

/* INC : Increment by one */
static inline void __mcs6502_execute_inc(struct mcs6502 *cpu) {
  uint8_t in_b = __mcs6502_fetch_data(cpu);

  __mcs6502_increase_common(cpu, &in_b);

  __mcs6502_write_bus_back_data(cpu, in_b);
}

/* INX : Increment X by one */
static inline void __mcs6502_execute_inx(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  __mcs6502_increase_common(cpu, &reg->X);
}

/* INY : Increment Y by one */
static inline void __mcs6502_execute_iny(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  __mcs6502_increase_common(cpu, &reg->Y);
}

/* JMP : Jump to new location */
static inline void __mcs6502_execute_jmp(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  reg->PC = cpu->bus_addr;
}

/* JSR : Jump to subroutine */
static inline void __mcs6502_execute_jsr(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  __mcs6502_push_word(cpu, reg->PC - 1);
  reg->PC = cpu->bus_addr;
}

/* LDA / LDX / LDY */
static inline void __mcs6502_load_common(struct mcs6502 *cpu, uint8_t *dest) {
  struct mcs6502_register *reg = &cpu->reg;

  *dest = __mcs6502_fetch_data(cpu);

  __mcs6502_update_P_Z(reg, *dest);
  __mcs6502_update_P_N(reg, *dest);
}

/* LDA : Load A */
static inline void __mcs6502_execute_lda(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  __mcs6502_load_common(cpu, &reg->A);
}

/* LDX : Load X */
static inline void __mcs6502_execute_ldx(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  __mcs6502_load_common(cpu, &reg->X);
}

/* LDY : Load Y */
static inline void __mcs6502_execute_ldy(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  __mcs6502_load_common(cpu, &reg->Y);
}

/* LSR : Logical shift right */
static inline void __mcs6502_execute_lsr(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;
  union {
    uint16_t w;
    uint8_t b[2];
  } in = { .w = 0, };

  in.b[1] = __mcs6502_fetch_data(cpu);
  in.w = in.w >> 1;

  reg->P.C = !!(in.b[0]);

  __mcs6502_update_P_Z(reg, in.b[1]);
  __mcs6502_update_P_N(reg, in.b[1]);

  __mcs6502_write_bus_back_data(cpu, in.b[1]);
}

/* NOP : No operation */
static inline void __mcs6502_execute_nop(struct mcs6502 *cpu) {
  cpu = cpu;
}

/* ORA : OR to A */
static inline void __mcs6502_execute_ora(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  reg->A |= __mcs6502_fetch_data(cpu);

  __mcs6502_update_P_Z(reg, reg->A);
  __mcs6502_update_P_N(reg, reg->A);
}

/* PHA : Push A onto stack */
static inline void __mcs6502_execute_pha(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  __mcs6502_push_byte(cpu, reg->A);
}

/* PHP : Push P onto stack */
static inline void __mcs6502_execute_php(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  reg->P.B = 1;
  __mcs6502_push_byte(cpu, reg->P.raw);
  reg->P.B = 0;
}

/* PLA : Pull (pop) A from stack */
static inline void __mcs6502_execute_pla(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  reg->A = __mcs6502_pop_byte(cpu);

  __mcs6502_update_P_Z(reg, reg->A);
  __mcs6502_update_P_N(reg, reg->A);
}

/* PLP : Pull (pop) P from stack */
static inline void __mcs6502_execute_plp(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  reg->P.raw = __mcs6502_pop_byte(cpu);
  /* FIXME: Why? */
  reg->P.U = 1;
}

/* ROL : Rotate left through carry */
static inline void __mcs6502_execute_rol(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;
  union {
    uint16_t w;
    uint8_t b[2];
  } in = { .w = 0, };

  in.b[0] = __mcs6502_fetch_data(cpu);
  in.w = in.w << 1 | reg->P.C;

  __mcs6502_update_P_C(reg, in.w);
  __mcs6502_update_P_Z(reg, in.b[0]);
  __mcs6502_update_P_N(reg, in.b[0]);

  __mcs6502_write_bus_back_data(cpu, in.b[0]);
}

/* ROR : Rotate right through carry */
static inline void __mcs6502_execute_ror(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;
  union {
    uint16_t w;
    uint8_t b[2];
  } in = { .w = 0, };

  in.b[1] = __mcs6502_fetch_data(cpu);
  in.w = in.w >> 1;
  in.b[1] |= reg->P.C << 7;

  reg->P.C = !!(in.b[0]);

  __mcs6502_update_P_Z(reg, in.b[1]);
  __mcs6502_update_P_N(reg, in.b[1]);

  __mcs6502_write_bus_back_data(cpu, in.b[1]);
}

/* RTI : Return from interrupt */
static inline void __mcs6502_execute_rti(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  reg->P.raw = __mcs6502_pop_byte(cpu);
  reg->PC = __mcs6502_pop_word(cpu);

  cpu->irq_state = E_MCS6502_IRQ_STATE_EXIT;
}

/* RTS : Return from subroutine */
static inline void __mcs6502_execute_rts(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  reg->PC = __mcs6502_pop_word(cpu);
  reg->PC++;
}

/* SBC : Subtract with borrow from A */
static inline void __mcs6502_execute_sbc(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;
  uint8_t in_b = ~__mcs6502_fetch_data(cpu);

  if (!reg->P.D) {
    union {
      uint16_t w;
      uint8_t b[2];
    } out;

    out.w = (uint16_t)reg->A + (uint16_t)in_b + (uint16_t)(reg->P.C);

    __mcs6502_update_P_N(reg, out.b[0]);
    __mcs6502_update_P_V(reg, out.w, reg->A, in_b);
    __mcs6502_update_P_Z(reg, out.b[0]);
    __mcs6502_update_P_C(reg, out.w);

    reg->A = out.b[0];
  } else {
    int8_t out;

    out = __mcs6502_from_bcd(reg->A) - __mcs6502_from_bcd(in_b) - !reg->P.C;

    reg->P.C = !!(out >= 0);

    if (out < 0)
      out += 100;

    reg->A = __mcs6502_to_bcd(out);

    __mcs6502_update_P_N(reg, out);
    __mcs6502_update_P_Z(reg, out);

    __mcs6502_increase_clk_per_instr(cpu, 1);
  }
}

/* SEC : Set carry */
static inline void __mcs6502_execute_sec(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  reg->P.C = 1;
}

/* SED : Set decimal mode */
static inline void __mcs6502_execute_sed(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  reg->P.D = 1;
}

/* SEI : Set IRQ disable */
static inline void __mcs6502_execute_sei(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  reg->P.I = 1;
}

/* STA : Store A */
static inline void __mcs6502_execute_sta(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  __mcs6502_write_bus_back_data(cpu, reg->A);
}

/* STX : Store X */
static inline void __mcs6502_execute_stx(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  __mcs6502_write_bus_back_data(cpu, reg->X);
}

/* STY : Store Y */
static inline void __mcs6502_execute_sty(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  __mcs6502_write_bus_back_data(cpu, reg->Y);
}

/* TAX : Transfer A to X */
static inline void __mcs6502_execute_tax(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  reg->X = reg->A;

  __mcs6502_update_P_Z(reg, reg->X);
  __mcs6502_update_P_N(reg, reg->X);
}

/* TAY : Transfer A to Y */
static inline void __mcs6502_execute_tay(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  reg->Y = reg->A;

  __mcs6502_update_P_Z(reg, reg->Y);
  __mcs6502_update_P_N(reg, reg->Y);
}

/* TSX : Transfer S to X */
static inline void __mcs6502_execute_tsx(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  reg->X = reg->S;

  __mcs6502_update_P_Z(reg, reg->X);
  __mcs6502_update_P_N(reg, reg->X);
}

/* TXA : Transfer X to A */
static inline void __mcs6502_execute_txa(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  reg->A = reg->X;

  __mcs6502_update_P_Z(reg, reg->A);
  __mcs6502_update_P_N(reg, reg->A);
}

/* TXS : Transfer X to S */
static inline void __mcs6502_execute_txs(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  reg->S = reg->X;
}

/* TYA : Transfer Y to A */
static inline void __mcs6502_execute_tya(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  reg->A = reg->Y;

  __mcs6502_update_P_Z(reg, reg->A);
  __mcs6502_update_P_N(reg, reg->A);
}

static inline void __mcs6502_nmi(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  __mcs6502_push_word(cpu, reg->PC);
  __mcs6502_push_byte(cpu, reg->P.raw);
  reg->P.I = 1;
  reg->PCL = __mcs6502_bus_read(cpu, MCS6502_NMI_VECTOR_LOW);
  reg->PCH = __mcs6502_bus_read(cpu, MCS6502_NMI_VECTOR_HIGH);

  cpu->irq_state = E_MCS6502_IRQ_STATE_IN;
  if (E_MCS6502_IRQ_PIN_H_L_H == cpu->nmi)
    cpu->nmi = E_MCS6502_IRQ_PIN_H;
}

static inline void __mcs6502_irq(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;

  __mcs6502_push_word(cpu, reg->PC);
  __mcs6502_push_byte(cpu, reg->P.raw);
  reg->P.I = 1;
  reg->PCL = __mcs6502_bus_read(cpu, MCS6502_IRQ_VECTOR_LOW);
  reg->PCH = __mcs6502_bus_read(cpu, MCS6502_IRQ_VECTOR_HIGH);

  cpu->irq_state = E_MCS6502_IRQ_STATE_IN;
  if (E_MCS6502_IRQ_PIN_H_L_H == cpu->nmi)
    cpu->nmi = E_MCS6502_IRQ_PIN_H;
}

/* main executions */
static inline int __mcs6502_execute(struct mcs6502 *cpu) {
  struct mcs6502_register *reg = &cpu->reg;
  uint16_t addr_to_fetch = reg->PC;
  uint8_t opcode;
  int hook_type;

  hook_type = e8bit_hook_handle(&cpu->hook, addr_to_fetch, cpu);
  if (hook_type == EMUL_8BIT_HOOK_BREAK_POINT)
    return MCS6502_CPU_PAUSE;

  if (reg->PC != addr_to_fetch)
    return 0;

  cpu->fetch_src = E_MCS6502_FETCH_SRC_BUS_ADDR;
  /* FIXME: why? */
  reg->P.U = 1;

  opcode = __mcs6502_fetch_pc(cpu);
  switch (opcode) {
    case 0x00:
      __mcs6502_access_none(cpu);
      __mcs6502_execute_brk(cpu);
      __mcs6502_set_clk_per_instr(cpu, 7);
      break;
    case 0x01:
      __mcs6502_access_indirect_x(cpu);
      __mcs6502_execute_ora(cpu);
      break;
    case 0x05:
      __mcs6502_access_zero_page(cpu);
      __mcs6502_execute_ora(cpu);
      break;
    case 0x06:
      __mcs6502_access_zero_page(cpu);
      __mcs6502_execute_asl(cpu);
      __mcs6502_set_clk_per_instr(cpu, 5);
      break;
    case 0x08:
      __mcs6502_access_none(cpu);
      __mcs6502_execute_php(cpu);
      __mcs6502_set_clk_per_instr(cpu, 3);
      break;
    case 0x09:
      __mcs6502_access_immediate(cpu);
      __mcs6502_execute_ora(cpu);
      break;
    case 0x0A:
      __mcs6502_access_accumulator(cpu);
      __mcs6502_execute_asl(cpu);
      break;
    case 0x0D:
      __mcs6502_access_absolute(cpu);
      __mcs6502_execute_ora(cpu);
      break;
    case 0x0E:
      __mcs6502_access_absolute(cpu);
      __mcs6502_execute_asl(cpu);
      __mcs6502_set_clk_per_instr(cpu, 6);
      break;
    case 0x10:
      __mcs6502_access_relative(cpu);
      __mcs6502_execute_bpl(cpu);
      break;
    case 0x11:
      __mcs6502_access_indirect_y(cpu);
      __mcs6502_execute_ora(cpu);
      break;
    case 0x15:
      __mcs6502_access_zero_page_x(cpu);
      __mcs6502_execute_ora(cpu);
      break;
    case 0x16:
      __mcs6502_access_zero_page_x(cpu);
      __mcs6502_execute_asl(cpu);
      __mcs6502_set_clk_per_instr(cpu, 6);
      break;
    case 0x18:
      __mcs6502_access_none(cpu);
      __mcs6502_execute_clc(cpu);
      break;
    case 0x19:
      __mcs6502_access_absolute_y(cpu);
      __mcs6502_execute_ora(cpu);
      break;
    case 0x1D:
      __mcs6502_access_absolute_x(cpu);
      __mcs6502_execute_ora(cpu);
      break;
    case 0x1E:
      __mcs6502_access_absolute_x(cpu);
      __mcs6502_execute_asl(cpu);
      __mcs6502_set_clk_per_instr(cpu, 7);
      break;
    case 0x20:
      __mcs6502_access_absolute(cpu);
      __mcs6502_execute_jsr(cpu);
      __mcs6502_set_clk_per_instr(cpu, 6);
      break;
    case 0x21:
      __mcs6502_access_indirect_x(cpu);
      __mcs6502_execute_and(cpu);
      break;
    case 0x24:
      __mcs6502_access_zero_page(cpu);
      __mcs6502_execute_bit(cpu);
      break;
    case 0x25:
      __mcs6502_access_zero_page(cpu);
      __mcs6502_execute_and(cpu);
      break;
    case 0x26:
      __mcs6502_access_zero_page(cpu);
      __mcs6502_execute_rol(cpu);
      __mcs6502_set_clk_per_instr(cpu, 5);
      break;
    case 0x28:
      __mcs6502_access_none(cpu);
      __mcs6502_execute_plp(cpu);
      __mcs6502_set_clk_per_instr(cpu, 4);
      break;
    case 0x29:
      __mcs6502_access_immediate(cpu);
      __mcs6502_execute_and(cpu);
      break;
    case 0x2A:
      __mcs6502_access_accumulator(cpu);
      __mcs6502_execute_rol(cpu);
      break;
    case 0x2C:
      __mcs6502_access_absolute(cpu);
      __mcs6502_execute_bit(cpu);
      break;
    case 0x2D:
      __mcs6502_access_absolute(cpu);
      __mcs6502_execute_and(cpu);
      break;
    case 0x2E:
      __mcs6502_access_absolute(cpu);
      __mcs6502_execute_rol(cpu);
      __mcs6502_set_clk_per_instr(cpu, 6);
      break;
    case 0x30:
      __mcs6502_access_relative(cpu);
      __mcs6502_execute_bmi(cpu);
      break;
    case 0x31:
      __mcs6502_access_indirect_y(cpu);
      __mcs6502_execute_and(cpu);
      break;
    case 0x35:
      __mcs6502_access_zero_page_x(cpu);
      __mcs6502_execute_and(cpu);
      break;
    case 0x36:
      __mcs6502_access_zero_page_x(cpu);
      __mcs6502_execute_rol(cpu);
      __mcs6502_set_clk_per_instr(cpu, 6);
      break;
    case 0x38:
      __mcs6502_access_none(cpu);
      __mcs6502_execute_sec(cpu);
      break;
    case 0x39:
      __mcs6502_access_absolute_y(cpu);
      __mcs6502_execute_and(cpu);
      break;
    case 0x3D:
      __mcs6502_access_absolute_x(cpu);
      __mcs6502_execute_and(cpu);
      break;
    case 0x3E:
      __mcs6502_access_absolute_x(cpu);
      __mcs6502_execute_rol(cpu);
      __mcs6502_set_clk_per_instr(cpu, 7);
      break;
    case 0x40:
      __mcs6502_access_none(cpu);
      __mcs6502_execute_rti(cpu);
      __mcs6502_set_clk_per_instr(cpu, 6);
      break;
    case 0x41:
      __mcs6502_access_indirect_x(cpu);
      __mcs6502_execute_eor(cpu);
      break;
    case 0x45:
      __mcs6502_access_zero_page(cpu);
      __mcs6502_execute_eor(cpu);
      break;
    case 0x46:
      __mcs6502_access_zero_page(cpu);
      __mcs6502_execute_lsr(cpu);
      __mcs6502_set_clk_per_instr(cpu, 5);
      break;
    case 0x48:
      __mcs6502_access_none(cpu);
      __mcs6502_execute_pha(cpu);
      __mcs6502_set_clk_per_instr(cpu, 3);
      break;
    case 0x49:
      __mcs6502_access_immediate(cpu);
      __mcs6502_execute_eor(cpu);
      break;
    case 0x4A:
      __mcs6502_access_accumulator(cpu);
      __mcs6502_execute_lsr(cpu);
      break;
    case 0x4C:
      __mcs6502_access_absolute(cpu);
      __mcs6502_execute_jmp(cpu);
      __mcs6502_set_clk_per_instr(cpu, 3);
      break;
    case 0x4D:
      __mcs6502_access_absolute(cpu);
      __mcs6502_execute_eor(cpu);
      break;
    case 0x4E:
      __mcs6502_access_absolute(cpu);
      __mcs6502_execute_lsr(cpu);
      __mcs6502_set_clk_per_instr(cpu, 6);
      break;
    case 0x50:
      __mcs6502_access_relative(cpu);
      __mcs6502_execute_bvc(cpu);
      break;
    case 0x51:
      __mcs6502_access_indirect_y(cpu);
      __mcs6502_execute_eor(cpu);
      break;
    case 0x55:
      __mcs6502_access_zero_page_x(cpu);
      __mcs6502_execute_eor(cpu);
      break;
    case 0x56:
      __mcs6502_access_zero_page_x(cpu);
      __mcs6502_execute_lsr(cpu);
      __mcs6502_set_clk_per_instr(cpu, 6);
      break;
    case 0x58:
      __mcs6502_access_none(cpu);
      __mcs6502_execute_cli(cpu);
      break;
    case 0x59:
      __mcs6502_access_absolute_y(cpu);
      __mcs6502_execute_eor(cpu);
      break;
    case 0x5D:
      __mcs6502_access_absolute_x(cpu);
      __mcs6502_execute_eor(cpu);
      break;
    case 0x5E:
      __mcs6502_access_absolute_x(cpu);
      __mcs6502_execute_lsr(cpu);
      __mcs6502_set_clk_per_instr(cpu, 7);
      break;
    case 0x60:
      __mcs6502_access_none(cpu);
      __mcs6502_execute_rts(cpu);
      __mcs6502_set_clk_per_instr(cpu, 6);
      break;
    case 0x61:
      __mcs6502_access_indirect_x(cpu);
      __mcs6502_execute_adc(cpu);
      break;
    case 0x65:
      __mcs6502_access_zero_page(cpu);
      __mcs6502_execute_adc(cpu);
      break;
    case 0x66:
      __mcs6502_access_zero_page(cpu);
      __mcs6502_execute_ror(cpu);
      __mcs6502_set_clk_per_instr(cpu, 5);
      break;
    case 0x68:
      __mcs6502_access_none(cpu);
      __mcs6502_execute_pla(cpu);
      __mcs6502_set_clk_per_instr(cpu, 4);
      break;
    case 0x69:
      __mcs6502_access_immediate(cpu);
      __mcs6502_execute_adc(cpu);
      break;
    case 0x6A:
      __mcs6502_access_accumulator(cpu);
      __mcs6502_execute_ror(cpu);
      break;
    case 0x6C:
      __mcs6502_access_indirect(cpu);
      __mcs6502_execute_jmp(cpu);
      break;
    case 0x6D:
      __mcs6502_access_absolute(cpu);
      __mcs6502_execute_adc(cpu);
      break;
    case 0x6E:
      __mcs6502_access_absolute(cpu);
      __mcs6502_execute_ror(cpu);
      __mcs6502_set_clk_per_instr(cpu, 6);
      break;
    case 0x70:
      __mcs6502_access_relative(cpu);
      __mcs6502_execute_bvs(cpu);
      break;
    case 0x71:
      __mcs6502_access_indirect_y(cpu);
      __mcs6502_execute_adc(cpu);
      break;
    case 0x75:
      __mcs6502_access_zero_page_x(cpu);
      __mcs6502_execute_adc(cpu);
      break;
    case 0x76:
      __mcs6502_access_zero_page_x(cpu);
      __mcs6502_execute_ror(cpu);
      __mcs6502_set_clk_per_instr(cpu, 6);
      break;
    case 0x78:
      __mcs6502_access_none(cpu);
      __mcs6502_execute_sei(cpu);
      break;
    case 0x79:
      __mcs6502_access_absolute_y(cpu);
      __mcs6502_execute_adc(cpu);
      break;
    case 0x7D:
      __mcs6502_access_absolute_x(cpu);
      __mcs6502_execute_adc(cpu);
      break;
    case 0x7E:
      __mcs6502_access_absolute_x(cpu);
      __mcs6502_execute_ror(cpu);
      __mcs6502_set_clk_per_instr(cpu, 7);
      break;
    case 0x81:
      __mcs6502_access_indirect_x(cpu);
      __mcs6502_execute_sta(cpu);
      break;
    case 0x84:
      __mcs6502_access_zero_page(cpu);
      __mcs6502_execute_sty(cpu);
      break;
    case 0x85:
      __mcs6502_access_zero_page(cpu);
      __mcs6502_execute_sta(cpu);
      break;
    case 0x86:
      __mcs6502_access_zero_page(cpu);
      __mcs6502_execute_stx(cpu);
      break;
    case 0x88:
      __mcs6502_access_none(cpu);
      __mcs6502_execute_dey(cpu);
      break;
    case 0x8A:
      __mcs6502_access_none(cpu);
      __mcs6502_execute_txa(cpu);
      break;
    case 0x8C:
      __mcs6502_access_absolute(cpu);
      __mcs6502_execute_sty(cpu);
      break;
    case 0x8D:
      __mcs6502_access_absolute(cpu);
      __mcs6502_execute_sta(cpu);
      break;
    case 0x8E:
      __mcs6502_access_absolute(cpu);
      __mcs6502_execute_stx(cpu);
      break;
    case 0x90:
      __mcs6502_access_relative(cpu);
      __mcs6502_execute_bcc(cpu);
      break;
    case 0x91:
      __mcs6502_access_indirect_y(cpu);
      __mcs6502_execute_sta(cpu);
      __mcs6502_set_clk_per_instr(cpu, 6);
      break;
    case 0x94:
      __mcs6502_access_zero_page_x(cpu);
      __mcs6502_execute_sty(cpu);
      break;
    case 0x95:
      __mcs6502_access_zero_page_x(cpu);
      __mcs6502_execute_sta(cpu);
      break;
    case 0x96:
      __mcs6502_access_zero_page_y(cpu);
      __mcs6502_execute_stx(cpu);
      break;
    case 0x98:
      __mcs6502_access_none(cpu);
      __mcs6502_execute_tya(cpu);
      break;
    case 0x99:
      __mcs6502_access_absolute_y(cpu);
      __mcs6502_execute_sta(cpu);
      __mcs6502_set_clk_per_instr(cpu, 5);
      break;
    case 0x9A:
      __mcs6502_access_none(cpu);
      __mcs6502_execute_txs(cpu);
      break;
    case 0x9D:
      __mcs6502_access_absolute_x(cpu);
      __mcs6502_execute_sta(cpu);
      __mcs6502_set_clk_per_instr(cpu, 5);
      break;
    case 0xA0:
      __mcs6502_access_immediate(cpu);
      __mcs6502_execute_ldy(cpu);
      break;
    case 0xA1:
      __mcs6502_access_indirect_x(cpu);
      __mcs6502_execute_lda(cpu);
      break;
    case 0xA2:
      __mcs6502_access_immediate(cpu);
      __mcs6502_execute_ldx(cpu);
      break;
    case 0xA4:
      __mcs6502_access_zero_page(cpu);
      __mcs6502_execute_ldy(cpu);
      break;
    case 0xA5:
      __mcs6502_access_zero_page(cpu);
      __mcs6502_execute_lda(cpu);
      break;
    case 0xA6:
      __mcs6502_access_zero_page(cpu);
      __mcs6502_execute_ldx(cpu);
      break;
    case 0xA8:
      __mcs6502_access_none(cpu);
      __mcs6502_execute_tay(cpu);
      break;
    case 0xA9:
      __mcs6502_access_immediate(cpu);
      __mcs6502_execute_lda(cpu);
      break;
    case 0xAA:
      __mcs6502_access_none(cpu);
      __mcs6502_execute_tax(cpu);
      break;
    case 0xAC:
      __mcs6502_access_absolute(cpu);
      __mcs6502_execute_ldy(cpu);
      break;
    case 0xAD:
      __mcs6502_access_absolute(cpu);
      __mcs6502_execute_lda(cpu);
      break;
    case 0xAE:
      __mcs6502_access_absolute(cpu);
      __mcs6502_execute_ldx(cpu);
      break;
    case 0xB0:
      __mcs6502_access_relative(cpu);
      __mcs6502_execute_bcs(cpu);
      break;
    case 0xB1:
      __mcs6502_access_indirect_y(cpu);
      __mcs6502_execute_lda(cpu);
      break;
    case 0xB4:
      __mcs6502_access_zero_page_x(cpu);
      __mcs6502_execute_ldy(cpu);
      break;
    case 0xB5:
      __mcs6502_access_zero_page_x(cpu);
      __mcs6502_execute_lda(cpu);
      break;
    case 0xB6:
      __mcs6502_access_zero_page_y(cpu);
      __mcs6502_execute_ldx(cpu);
      break;
    case 0xB8:
      __mcs6502_access_none(cpu);
      __mcs6502_execute_clv(cpu);
      break;
    case 0xB9:
      __mcs6502_access_absolute_y(cpu);
      __mcs6502_execute_lda(cpu);
      break;
    case 0xBA:
      __mcs6502_access_none(cpu);
      __mcs6502_execute_tsx(cpu);
      break;
    case 0xBC:
      __mcs6502_access_absolute_x(cpu);
      __mcs6502_execute_ldy(cpu);
      break;
    case 0xBD:
      __mcs6502_access_absolute_x(cpu);
      __mcs6502_execute_lda(cpu);
      break;
    case 0xBE:
      __mcs6502_access_absolute_y(cpu);
      __mcs6502_execute_ldx(cpu);
      break;
    case 0xC0:
      __mcs6502_access_immediate(cpu);
      __mcs6502_execute_cpy(cpu);
      break;
    case 0xC1:
      __mcs6502_access_indirect_x(cpu);
      __mcs6502_execute_cmp(cpu);
      break;
    case 0xC4:
      __mcs6502_access_zero_page(cpu);
      __mcs6502_execute_cpy(cpu);
      break;
    case 0xC5:
      __mcs6502_access_zero_page(cpu);
      __mcs6502_execute_cmp(cpu);
      break;
    case 0xC6:
      __mcs6502_access_zero_page(cpu);
      __mcs6502_execute_dec(cpu);
      __mcs6502_set_clk_per_instr(cpu, 5);
      break;
    case 0xC8:
      __mcs6502_access_none(cpu);
      __mcs6502_execute_iny(cpu);
      break;
    case 0xC9:
      __mcs6502_access_immediate(cpu);
      __mcs6502_execute_cmp(cpu);
      break;
    case 0xCA:
      __mcs6502_access_none(cpu);
      __mcs6502_execute_dex(cpu);
      break;
    case 0xCC:
      __mcs6502_access_absolute(cpu);
      __mcs6502_execute_cpy(cpu);
      break;
    case 0xCD:
      __mcs6502_access_absolute(cpu);
      __mcs6502_execute_cmp(cpu);
      break;
    case 0xCE:
      __mcs6502_access_absolute(cpu);
      __mcs6502_execute_dec(cpu);
      __mcs6502_set_clk_per_instr(cpu, 6);
      break;
    case 0xD0:
      __mcs6502_access_relative(cpu);
      __mcs6502_execute_bne(cpu);
      break;
    case 0xD1:
      __mcs6502_access_indirect_y(cpu);
      __mcs6502_execute_cmp(cpu);
      break;
    case 0xD5:
      __mcs6502_access_zero_page_x(cpu);
      __mcs6502_execute_cmp(cpu);
      break;
    case 0xD6:
      __mcs6502_access_zero_page_x(cpu);
      __mcs6502_execute_dec(cpu);
      __mcs6502_set_clk_per_instr(cpu, 6);
      break;
    case 0xD8:
      __mcs6502_access_none(cpu);
      __mcs6502_execute_cld(cpu);
      break;
    case 0xD9:
      __mcs6502_access_absolute_y(cpu);
      __mcs6502_execute_cmp(cpu);
      break;
    case 0xDD:
      __mcs6502_access_absolute_x(cpu);
      __mcs6502_execute_cmp(cpu);
      break;
    case 0xDE:
      __mcs6502_access_absolute_x(cpu);
      __mcs6502_execute_dec(cpu);
      __mcs6502_set_clk_per_instr(cpu, 7);
      break;
    case 0xE0:
      __mcs6502_access_immediate(cpu);
      __mcs6502_execute_cpx(cpu);
      break;
    case 0xE1:
      __mcs6502_access_indirect_x(cpu);
      __mcs6502_execute_sbc(cpu);
      break;
    case 0xE4:
      __mcs6502_access_zero_page(cpu);
      __mcs6502_execute_cpx(cpu);
      break;
    case 0xE5:
      __mcs6502_access_zero_page(cpu);
      __mcs6502_execute_sbc(cpu);
      break;
    case 0xE6:
      __mcs6502_access_zero_page(cpu);
      __mcs6502_execute_inc(cpu);
      __mcs6502_set_clk_per_instr(cpu, 5);
      break;
    case 0xE8:
      __mcs6502_access_none(cpu);
      __mcs6502_execute_inx(cpu);
      break;
    case 0xE9:
      __mcs6502_access_immediate(cpu);
      __mcs6502_execute_sbc(cpu);
      break;
    case 0xEA:
      __mcs6502_access_none(cpu);
      __mcs6502_execute_nop(cpu);
      break;
    case 0xEC:
      __mcs6502_access_absolute(cpu);
      __mcs6502_execute_cpx(cpu);
      break;
    case 0xED:
      __mcs6502_access_absolute(cpu);
      __mcs6502_execute_sbc(cpu);
      break;
    case 0xEE:
      __mcs6502_access_absolute(cpu);
      __mcs6502_execute_inc(cpu);
      __mcs6502_set_clk_per_instr(cpu, 6);
      break;
    case 0xF0:
      __mcs6502_access_relative(cpu);
      __mcs6502_execute_beq(cpu);
      break;
    case 0xF1:
      __mcs6502_access_indirect_y(cpu);
      __mcs6502_execute_sbc(cpu);
      break;
    case 0xF5:
      __mcs6502_access_zero_page_x(cpu);
      __mcs6502_execute_sbc(cpu);
      break;
    case 0xF6:
      __mcs6502_access_zero_page_x(cpu);
      __mcs6502_execute_inc(cpu);
      __mcs6502_set_clk_per_instr(cpu, 6);
      break;
    case 0xF8:
      __mcs6502_access_none(cpu);
      __mcs6502_execute_sed(cpu);
      break;
    case 0xF9:
      __mcs6502_access_absolute_y(cpu);
      __mcs6502_execute_sbc(cpu);
      break;
    case 0xFD:
      __mcs6502_access_absolute_x(cpu);
      __mcs6502_execute_sbc(cpu);
      break;
    case 0xFE:
      __mcs6502_access_absolute_x(cpu);
      __mcs6502_execute_inc(cpu);
      __mcs6502_set_clk_per_instr(cpu, 7);
      break;
  }

  return 0;
}

static inline bool __mcs6502_exec_continue(struct mcs6502 *cpu) {
#ifdef CFG_MCS6502_USE_TIMING
  if (cpu->clk_goal <= 0)
    return false;
#else
  if (!cpu->tickcount--)
    return false;
#endif

  return true;
}

static inline void __mcs6502_init_clk_goal(struct mcs6502 *cpu,
    unsigned long tickcount) {
#ifdef CFG_MCS6502_USE_TIMING
  cpu->clk_goal += tickcount;
#else
  cpu->tickcount = tickcount;
#endif
}

static inline void __mcs6502_update_clk_goal(struct mcs6502 *cpu) {
#ifdef CFG_MCS6502_USE_TIMING
  cpu->clk_goal -= cpu->clk_per_instr;
  cpu->cnt_instr++;
#else
  cpu = cpu;
#endif
}

int mcs6502_exec(struct mcs6502 *cpu, unsigned long tickcount) {
  struct mcs6502_register *reg = &cpu->reg;
  int ret;

  __mcs6502_init_clk_goal(cpu, tickcount);

  while (__mcs6502_exec_continue(cpu)) {
    ret = __mcs6502_execute(cpu);
    if (!ret)
      return ret;

    __mcs6502_update_clk_goal(cpu);

    if (cpu->irq_state == E_MCS6502_IRQ_STATE_IN) {
      continue;
    } else if (cpu->irq_state == E_MCS6502_IRQ_STATE_EXIT) {
      cpu->irq_state = E_MCS6502_IRQ_STATE_OUT;
      continue;
    }

    if (cpu->nmi != E_MCS6502_IRQ_PIN_H)
      __mcs6502_nmi(cpu);

    if ((cpu->irq != E_MCS6502_IRQ_PIN_H) && !reg->P.I)
      __mcs6502_irq(cpu);
  }

  return 0;
}

void mcs6502_change_nmi(struct mcs6502 *cpu, enum mcs6502_irq_pin_t state) {
  cpu->nmi = state;
}

void mcs6502_change_irq(struct mcs6502 *cpu, enum mcs6502_irq_pin_t state) {
  cpu->irq = state;
}
