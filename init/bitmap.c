#include <danux/mm.h>
#include <danux/page.h>
#include <danux/panic.h>
#include <stdint.h>

static uint64_t bitmap_start;
static uint64_t bitmap_end;
static uint64_t bitmap_sz;
static uint64_t *bitmap;

/*
 * find_bitmap_area: Sets the global varaibles bitmap_start and bitmap_end to mark the bitmap area.
 * This function should only be called once.
 */

static void find_bitmap_area(uint64_t sz)
{
	if (bitmap)
		panic("Bitmap area is already allocated");

	// Page-align the sz variable just in case.
	sz = (sz + PAGE_SIZE - 1) / PAGE_SIZE * PAGE_SIZE;

	for (int i = 0; i < usable_region_count; i++) {
		if (usable_regions[i].length >= sz) {
			// bitmap_start, bitmap_end store physical addresses.
			// bitmap_sz stores the size of the bitmap in bytes.
			// For the bitmap pointer, the HHDM mapping is already established when limine hands off control, so the virtual address must be stored.
			bitmap_start = usable_regions[i].base;
			bitmap_end = bitmap_start + sz;
			bitmap_sz = sz;
			bitmap = phys_to_virt(bitmap_start);
			return;
		}
	}
	
	panic("No memory is available to find bitmap area");
}

/*
 * bitmap_set: Sets all bits within [from, to) to 1.
 * Note that from is inclusive, to is exclusive.
 * bitmap_unset is analogous to bitmap_set.
 */

static inline void bitmap_set_single(uint64_t idx) {
	bitmap[idx/64] |= 1UL << (idx%64);
}

// Same as bitmap_set_single, only difference being that it unsets the target bit.

static inline void bitmap_unset_single(uint64_t idx) {
	bitmap[idx/64] &= ~(1UL << (idx%64));
}

static void bitmap_set(uint64_t from, uint64_t to) {
	if (to > bitmap_sz * 8 || from >= to)
		panic("Invalid arguments for bitmap_set");

	for (uint64_t i = from; i < to; i++)
		bitmap_set_single(i);
}

static void bitmap_unset(uint64_t from, uint64_t to) {
	if (to > bitmap_sz * 8 || from >= to)
		panic("Invalid arguments for bitmap_unset");

	for (uint64_t i = from; i < to; i++)
		bitmap_unset_single(i);
}

void bitmap_init(void) {
	uint64_t max_pfn = 0;
	for (int i = 0; i < usable_region_count; i++) {
		uint64_t tmp = (usable_regions[i].base + usable_regions[i].length) >> PAGE_SHIFT;
		if (tmp > max_pfn) max_pfn = tmp;
	}

	find_bitmap_area((max_pfn+7)/8); // The size of the bitmap should be byte-aligned.
	memset(bitmap, 0xFF, bitmap_sz);

	for (int i = 0; i < usable_region_count; i++) {
		bitmap_unset(
			usable_regions[i].base >> PAGE_SHIFT,
			(usable_regions[i].base + usable_regions[i].length) >> PAGE_SHIFT
		);
	}

	bitmap_set(bitmap_start >> PAGE_SHIFT, bitmap_end >> PAGE_SHIFT);
}
