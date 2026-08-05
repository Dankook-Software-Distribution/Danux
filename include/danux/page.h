#ifndef DANUX_PAGE_H
#define DANUX_PAGE_H

#include <danux/list.h>
#include <stdint.h>

#define PAGE_SHIFT 12
#define PAGE_SIZE (1 << PAGE_SHIFT)

extern uint64_t max_pfn;
extern struct page *page_arr;

extern void find_max_pfn(void);
extern void page_init(void);

// Page flag values. These definitions are not final.
#define PG_RESERVED	(1 << 0)
#define PG_ALLOCATED	(1 << 1)
#define PG_PROTECTED	(1 << 2)
#define PG_BUDDY	(1 << 3)

struct page {
	uint64_t flags;
	uint64_t misc; // Used to store miscellaneous values. The buddy allocator may use it to store the allocation order.
	struct list_head linkage; // Used to manage free pages by the buddy allocator.
};

#endif
