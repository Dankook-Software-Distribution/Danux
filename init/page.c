#include <danux/mm.h>
#include <danux/page.h>
#include <stdint.h>

uint64_t max_pfn;

void find_max_pfn(void) {
	for (uint64_t i = 0; i < usable_region_count; i++) {
		uint64_t tmp = (usable_regions[i].base + usable_regions[i].length) >> PAGE_SHIFT;
		if (tmp > max_pfn) max_pfn = tmp;
	}
}
