#ifndef DANUX_BITMAP_H
#define DANUX_BITMAP_H

#include <stdint.h>
#include <stdbool.h>

extern bool bitmap_test_single(uint64_t);
extern void bitmap_init(void);
extern void *bitmap_alloc(uint64_t);
extern void bitmap_free(void *, uint64_t);

#endif
