// SPDX-License-Identifier: GPL-2.0
#include <ctype.h>

#include "inc/a1ino.h"

static inline struct mc6820 *__tty_io_dev_to_pia(struct e8bit_io_dev *io_dev) {
  return container_of(io_dev->io, struct mc6820, io);
}

static inline struct a1ino_tty *__tty_io_dev_to_tty(struct e8bit_io_dev *io_dev) {
  return container_of(io_dev, struct a1ino_tty, io_dev);
}

static inline void ____tty_println(void) {
  while (!A1INO_Serial.println()) ;
}

static inline void ____tty_putchar(int tx) {
  switch (tx) {
    case 0x08:                 /* 'BS' */
    case 0x09:                 /* 'TAB' */
    case 0x12:                 /* 'LF' */
    case 0x13:                 /* 'VT' */
    case 0x20 ... 0x7E:        /* 'space' to '~' */
      break;
    default:
      return;
  }

  while (!A1INO_Serial.write((char)tx)) ;
}

static void __tty_putchar(struct a1ino_tty *tty, int tx) {
  if (tx == '\r')
    ____tty_println();
  else
    ____tty_putchar(tx);
}

static int __tty_getchar(struct a1ino_tty *tty) {
  int rx = 0;

  if (!tty->rx_buf.isEmpty())
    tty->rx_buf.pop(rx);

  return rx;
}

static void __pia_read_dsp(struct e8bit_io_dev *io_dev, uint8_t *mem, uint16_t offset) {
  struct mc6820 *pia = __tty_io_dev_to_pia(io_dev);

  /* NOTE: always completed */
  pia->reg.orb &= 0x7F;         /* Video DA pin */
  pia->reg.irqb1 = 0x0;         /* Video nRDA pin */
}

static void __pia_write_dsp(struct e8bit_io_dev *io_dev, uint8_t *mem, uint16_t offset) {
  struct mc6820 *pia = __tty_io_dev_to_pia(io_dev);
  struct a1ino_tty *tty = __tty_io_dev_to_tty(io_dev);
  int tx;

  tx = pia->reg.orb & 0x7F;

  __tty_putchar(tty, tx);
}

static void __pia_read_kbd(struct e8bit_io_dev *io_dev, uint8_t *mem, uint16_t offset) {
  struct mc6820 *pia = __tty_io_dev_to_pia(io_dev);
  struct a1ino_tty *tty = __tty_io_dev_to_tty(io_dev);

  if (tty->kbd_clr_flag)
    return;

  tty->kbd_clr_flag = false;
  pia->reg.ora = (uint8_t)(__tty_getchar(tty) | 0x80);
}

static void __pia_read_kbdcr(struct e8bit_io_dev *io_dev, uint8_t *mem, uint16_t offset) {
  struct mc6820 *pia = __tty_io_dev_to_pia(io_dev);
  struct a1ino_tty *tty = __tty_io_dev_to_tty(io_dev);

  /* Keyboard STR pin */
  pia->reg.irqa1 = tty->rx_buf.isEmpty()? 0 : 1;
}

static void __pia_write_kbdcr(struct e8bit_io_dev *io_dev, uint8_t *mem, uint16_t offset) {
  struct mc6820 *pia = __tty_io_dev_to_pia(io_dev);
  struct a1ino_tty *tty = __tty_io_dev_to_tty(io_dev);

  /* Keyboard CLR pin */
  if (pia->reg.irqa2)
    tty->kbd_clr_flag = true;
}

static void a1ino_tty_read(struct e8bit_io_dev *io_dev, uint8_t *mem, uint16_t offset) {
  if (offset == MC6820_IO_OFFSET_ORA)
    __pia_read_kbd(io_dev, mem, offset);
  else if (offset == MC6820_IO_OFFSET_CRA)
    __pia_read_kbdcr(io_dev, mem, offset);
  else if (offset == MC6820_IO_OFFSET_ORB)
    __pia_read_dsp(io_dev, mem, offset);
}

static void a1ino_tty_write(struct e8bit_io_dev *io_dev, uint8_t *mem, uint16_t offset) {
  if (offset == MC6820_IO_OFFSET_CRA)
    __pia_write_kbdcr(io_dev, mem, offset);
  else if (offset == MC6820_IO_OFFSET_ORB)
    __pia_write_dsp(io_dev, mem, offset);
}

static struct a1ino_tty *__tty_get_instance(void) {
  static struct a1ino_tty tty;

  tty.io_dev.read = a1ino_tty_read;
  tty.io_dev.write = a1ino_tty_write;

  return &tty;
}

struct a1ino_tty *a1ino_tty_get_instance(void) {
  static struct a1ino_tty *tty;

  if (tty)
    return tty;

  tty = __tty_get_instance();

  return tty;
}

void a1ino_tty_install(struct a1ino *emul) {
  struct a1ino_tty *tty = emul->tty;
  struct mc6820 *pia = a1ino_get_pia(emul);

  e8bit_io_attach_dev(&pia->io, &tty->io_dev);
}

void a1ino_tty_uninstall(struct a1ino *emul) {
  struct a1ino_tty *tty = emul->tty;
  struct mc6820 *pia = a1ino_get_pia(emul);

  e8bit_io_detach_dev(&pia->io, &tty->io_dev);
}

#define A1_INO_TTY_CTRL_R   0x12

void a1ino_tty_reset(struct a1ino *emul) {
  struct a1ino_tty *tty = emul->tty;

  tty->rx_buf.clear();
}

void a1ino_tty_run(struct a1ino *emul) {
  struct a1ino_tty *tty = emul->tty;

  while (A1INO_Serial.available() && !tty->rx_buf.isFull()) {
    int rx = toupper(A1INO_Serial.read());

    if (rx & 0x80)
      continue;

    if (rx == A1_INO_TTY_CTRL_R) {
      emul->state_machine = a1ino_state_reset;
      return;
    } else
      tty->rx_buf.push(rx);
  }
}
