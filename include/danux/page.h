#ifndef DANUX_PAGE_H
#define DANUX_PAGE_H

#include <stdint.h>

#define PAGE_SHIFT 12
#define PAGE_SIZE (1 << PAGE_SHIFT)

extern uint64_t max_pfn;

extern void find_max_pfn(void);

#define PG_RESERVED	(1 << 0)

struct page {
	uint64_t flags;
	uint64_t misc; // Used to store miscellaneous values. The buddy allocator may use it to store the allocation order.
	struct page *prev;
	struct page *next;
};

#endif
