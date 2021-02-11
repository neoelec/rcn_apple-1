#ifndef __EMUL_8BIT_NOTIFY_H__
#define __EMUL_8BIT_NOTIFY_H__

#include <stdint.h>

struct e8bit_notify_handle;

typedef void (*e8bit_notify_call_t)(struct e8bit_notify_handle *, unsigned int, void *);

struct e8bit_notify {
  struct list_head head;
};

struct e8bit_notify_handle {
  struct list_head list;
  e8bit_notify_call_t call;
};

#ifdef __cplusplus
extern "C" {
#endif

extern void e8bit_notify_setup(struct e8bit_notify *notify);
extern void e8bit_notify_attach_notify(struct e8bit_notify *notify, struct e8bit_notify_handle *handle);
extern void e8bit_notify_detach(struct e8bit_notify *notify, struct e8bit_notify_handle *handle);
extern void e8bit_notify_handle(struct e8bit_notify *notify, unsigned int i, void *d);

#ifdef __cplusplus
}
#endif

#endif /* __EMUL_8BIT_NOTIFY_H__ */
