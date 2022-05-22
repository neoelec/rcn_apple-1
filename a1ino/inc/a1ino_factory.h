#ifndef __A1INO_FACTORY_H__
#define __A1INO_FACTORY_H__

extern void a1ino_print_supported_product(void);
extern struct a1ino *a1ino_factory(size_t prod_id);
extern void a1ino_creator_template(struct a1ino *emul, a1ino_rom *(*concrete_creator)(void));
extern size_t a1ino_get_nr_prod(void);
extern void a1ino_describe_template(size_t idx, a1ino_rom *(*concrete_creator)(void)) ;

#endif /* __A1INO_FACTORY_H__ */
