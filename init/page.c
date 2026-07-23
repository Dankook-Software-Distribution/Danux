#include <danux/bitmap.h>
#include <danux/mm.h>
#include <danux/page.h>
#include <stdint.h>

uint64_t max_pfn;
struct page *page_arr;

void find_max_pfn(void) {
	for (uint64_t i = 0; i < usable_region_count; i++) {
		uint64_t tmp = (usable_regions[i].base + usable_regions[i].length) >> PAGE_SHIFT;
		if (tmp > max_pfn) max_pfn = tmp;
	}
}

void page_init(void) {
	page_arr = bitmap_alloc(max_pfn * sizeof(struct page));
	memset(page_arr, 0, max_pfn * sizeof(struct page));
	for (uint64_t i = 0; i < max_pfn; i++) {
		// Mark all pages as reserved. Page structs shouldn't be used until handed to the buddy allocator.
		page_arr[i].flags |= PG_RESERVED;
	}
}
