#ifndef __A1INO_FACTORY_H__
#define __A1INO_FACTORY_H__

extern struct a1ino *a1ino_factory(size_t prod_id);
extern void a1ino_creator_template(struct a1ino *emul, void (*concrete_creator)(struct a1ino *));
extern size_t a1ino_get_nr_prod(void);

#endif /* __A1INO_FACTORY_H__ */
