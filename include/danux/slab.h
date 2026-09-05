#ifndef DANUX_SLAB_H
#define DANUX_SLAB_H

#include <stdint.h>

extern void kmalloc_init(void);
extern void *kmalloc(uint64_t size);
extern void kfree(void *ptr);

#endif
