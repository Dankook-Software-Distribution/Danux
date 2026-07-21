#ifndef DANUX_MM_H
#define DANUX_MM_H

#include <limine.h>
#include <stdint.h>

#define MAX_USABLE_REGIONS	64	// 강제로 정한 최대 usable_regions의 개수

struct usable_region {
	uint64_t base;
	uint64_t length;
};

extern struct usable_region usable_regions[MAX_USABLE_REGIONS];
extern uint64_t usable_region_count;
extern uint64_t hhdm_offset;

extern uint64_t *phys_to_virt(uint64_t);
extern void memset(void *, int, uint64_t);

#endif
