#ifndef __INTERNAL__APPLE_1_PIA_H__
#define __INTERNAL__APPLE_1_PIA_H__

extern void apple_1_pia_setup(struct mc6820 *pia);
extern void apple_1_pia_install(struct mc6820 *pia, struct e8bit_bus *bus);
extern void apple_1_pia_uninstall(struct mc6820 *pia, struct e8bit_bus *bus);

#endif /* __INTERNAL__APPLE_1_PIA_H__ */
