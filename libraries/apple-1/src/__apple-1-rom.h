#ifndef __INTERNAL__APPLE_1_ROM_H__
#define __INTERNAL__APPLE_1_ROM_H__

extern void apple_1_rom_setup(struct apple_1_rom *rom);
extern void apple_1_rom_install(struct apple_1_rom *rom, struct e8bit_bus *bus);
extern void apple_1_rom_uninstall(struct apple_1_rom *rom, struct e8bit_bus *bus);

#endif /* __INTERNAL__APPLE_1_ROM_H__ */
