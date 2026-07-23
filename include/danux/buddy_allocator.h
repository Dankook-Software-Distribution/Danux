#ifndef DANUX_BUDDY_ALLOCATOR_H
#define DANUX_BUDDY_ALLOCATOR_H

#include <danux/page.h>
#include <stdint.h>

uint64_t *page_alloc();
void page_free(struct page *page_arr);

#endif
