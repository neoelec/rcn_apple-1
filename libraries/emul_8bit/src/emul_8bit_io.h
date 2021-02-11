#ifndef __EMUL_8BIT_IO_H__
#define __EMUL_8BIT_IO_H__

#include <stddef.h>
#include <stdint.h>

#include "emul_8bit_bus.h"

struct e8bit_io;
struct e8bit_io_dev;

typedef void (*e8bit_io_dev_handle_t)(struct e8bit_io_dev *, uint8_t *, uint16_t);

struct e8bit_io_dev {
  struct list_head list;
  struct e8bit_io *io;
  e8bit_io_dev_handle_t read;
  e8bit_io_dev_handle_t write;
};

struct e8bit_io {
  struct e8bit_bus_dev dev;
  struct list_head head;
  uint8_t *mem;
};

#ifdef __cplusplus
extern "C" {
#endif

extern void e8bit_io_attach_dev(struct e8bit_io *io, struct e8bit_io_dev *dev);
extern void e8bit_io_detach_dev(struct e8bit_io *io, struct e8bit_io_dev *dev);
extern void e8bit_io_setup(struct e8bit_io *io, uint16_t base, uint16_t size, uint8_t *mem);

#ifdef __cplusplus
}
#endif

#endif /* __EMUL_8BIT_IO_H__ */
