#ifndef __INTERNAL__APPLE_1_RAM_H__
#define __INTERNAL__APPLE_1_RAM_H__

extern void apple_1_ram_setup(struct apple_1_ram *ram);
extern void apple_1_ram_install(struct apple_1_ram *ram, struct e8bit_bus *bus);
extern void apple_1_ram_uninstall(struct apple_1_ram *ram, struct e8bit_bus *bus);

#endif /* __INTERNAL__APPLE_1_RAM_H__ */
