#ifndef __A1INO_TELETYPE_H__
#define __A1INO_TELETYPE_H__

#include <RingBuf.h>

#include "a1ino.h"

#define A1INO_TTY_RX_BUF_SZ  64

struct a1ino_tty {
  struct e8bit_io_dev io_dev;
  RingBuf<int, A1INO_TTY_RX_BUF_SZ> rx_buf;
  bool kbd_clr_flag;
};

extern struct a1ino_tty *a1ino_tty_get_instance(void);

extern void a1ino_tty_install(struct a1ino *emul);
extern void a1ino_tty_uninstall(struct a1ino *emul);

extern void a1ino_tty_reset(struct a1ino *emul);
extern void a1ino_tty_run(struct a1ino *emul);

#endif // __A1INO_TELETYPE_H__
