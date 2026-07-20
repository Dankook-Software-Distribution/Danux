#ifndef MM_H
#define MM_H

#include <limine.h>
#include <stdint.h>

#define MAX_USABLE_REGIONS	64	// 강제로 정한 최대 usable_regions의 개수

struct usable_region {
	uint64_t base;
	uint64_t length;
};

struct usable_region usable_regions[MAX_USABLE_REGIONS];
uint64_t usable_region_count;

#endif
