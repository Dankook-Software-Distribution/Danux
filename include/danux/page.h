#ifndef DANUX_PAGE_H
#define DANUX_PAGE_H

#include <stdint.h>

#define PAGE_SHIFT 12
#define PAGE_SIZE (1 << PAGE_SHIFT)

extern uint64_t max_pfn;

extern void find_max_pfn(void);

#endif
