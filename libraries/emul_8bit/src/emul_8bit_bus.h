#ifndef __EMUL_8BIT_BUS_H__
#define __EMUL_8BIT_BUS_H__

#include <rbtree.h>
#include <stdbool.h>
#include <stdint.h>

enum e8bit_bus_type {
  E_E8BIT_BUS_TYPE_RAM = 0,
  E_E8BIT_BUS_TYPE_ROM,
  E_E8BIT_BUS_TYPE_IO,
};

struct e8bit_bus_dev;

typedef uint8_t(*e8bit_bus_read_t) (const struct e8bit_bus_dev *, uint16_t);
typedef void (*e8bit_bus_write_t)(const struct e8bit_bus_dev *, uint16_t, uint8_t);

struct e8bit_bus_dev {
  struct rb_node node;
  uint16_t base;
  uint16_t end;
  e8bit_bus_read_t read;
  e8bit_bus_write_t write;
  enum e8bit_bus_type type;
};

struct e8bit_bus {
  struct rb_root root;
};

static inline bool e8bit_bus_read_dev(struct e8bit_bus_dev *dev,
    uint16_t addr, uint8_t *val) {
  if ((addr >= dev->base) && (addr <= dev->end))
    goto matched;

  return false;

matched:
  if (dev->read) {
    *val = dev->read(dev, addr);
    return true;
  }

  return false;
}

static inline bool e8bit_bus_write_dev(struct e8bit_bus_dev *dev,
    uint16_t addr, uint8_t val) {
  if ((addr >= dev->base) && (addr <= dev->end))
    goto matched;

  return false;

matched:
  if (dev->write) {
    dev->write(dev, addr, val);
    return true;
  }

  return false;
}

#ifdef __cplusplus
extern "C" {
#endif

extern void e8bit_bus_setup(struct e8bit_bus *bus);
extern void e8bit_bus_attach(struct e8bit_bus *bus, struct e8bit_bus_dev *dev);
extern void e8bit_bus_detach(struct e8bit_bus *bus, struct e8bit_bus_dev *dev);
extern struct e8bit_bus_dev *e8bit_bus_read(struct e8bit_bus *bus, uint16_t addr, uint8_t *val);
extern struct e8bit_bus_dev *e8bit_bus_write(struct e8bit_bus *bus, uint16_t addr, uint8_t val);

#ifdef __cplusplus
}
#endif

#endif /* __EMUL_8BIT_BUS_H__ */
