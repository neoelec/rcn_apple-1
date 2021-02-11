#ifndef __EMUL_8BIT_HOOK_H__
#define __EMUL_8BIT_HOOK_H__

#include <hashtable.h>
#include <stdbool.h>
#include <stdint.h>

#define EMUL_8BIT_HOOK_CONTINUE     0
#define EMUL_8BIT_HOOK_BREAK_POINT  1

struct e8bit_hook_handle;

typedef int (*e8bit_hook_call_t)(struct e8bit_hook_handle *, void *);

struct e8bit_hook {
  DECLARE_HASHTABLE(htbl, 4);
};

struct e8bit_hook_handle {
  struct hlist_node node;
  uint16_t ptr;
  e8bit_hook_call_t call;
};

#ifdef __cplusplus
extern "C" {
#endif

extern void e8bit_hook_setup(struct e8bit_hook *hook);
extern void e8bit_hook_attach(struct e8bit_hook *hook, struct e8bit_hook_handle *handle);
extern void e8bit_hook_detach(struct e8bit_hook *hook, struct e8bit_hook_handle *handle);
extern int e8bit_hook_handle(struct e8bit_hook *hook, uint16_t addr, void *d);

#ifdef __cplusplus
}
#endif

#endif /* __EMUL_8BIT_HOOK_H__ */
