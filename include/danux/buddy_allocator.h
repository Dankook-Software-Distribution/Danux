#ifndef DANUX_BUDDY_ALLOCATOR_H
#define DANUX_BUDDY_ALLOCATOR_H

#include <danux/page.h>
#include <stdint.h>

#define MAX_ORDER	10

typedef struct buddy_system buddy_system;

void *page_alloc(uint64_t byte_size);
void page_free(void *addr);
void buddy_init();

#endif
