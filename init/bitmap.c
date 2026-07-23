#include <danux/bitmap.h>
#include <danux/mm.h>
#include <danux/page.h>
#include <danux/panic.h>
#include <stdbool.h>
#include <stdint.h>

static uint64_t bitmap_start;
static uint64_t bitmap_end;
static uint64_t bitmap_sz;
static uint64_t *bitmap;
static uint64_t bitmap_hint; // This is a heuristic bitmap index variable. It indicates that allocation will likely succeed if the search starts from the stored index.

/*
 * find_bitmap_area: Sets the global varaibles bitmap_start and bitmap_end to mark the bitmap area.
 * The argument sz denotes the size of the bitmap in bytes.
 * This function should only be called once.
 */
static void find_bitmap_area(uint64_t sz)
{
	if (bitmap)
		panic("Bitmap area is already allocated");

	// Page-align the sz variable just in case.
	sz = (sz + PAGE_SIZE - 1) / PAGE_SIZE * PAGE_SIZE;

	for (uint64_t i = 0; i < usable_region_count; i++) {
		if (usable_regions[i].length >= sz) {
			// bitmap_start, bitmap_end store physical addresses.
			// bitmap_sz stores the size of the bitmap in bytes.
			// For the bitmap pointer, the HHDM mapping is already established when limine hands off control, so the virtual address must be stored.
			bitmap_start = usable_regions[i].base;
			bitmap_end = bitmap_start + sz;
			bitmap_sz = sz;
			bitmap = phys_to_virt(bitmap_start);
			bitmap_hint = 0;
			return;
		}
	}
	
	panic("No memory is available to find bitmap area");
}

static inline void bitmap_set_single(uint64_t idx) {
	bitmap[idx/64] |= 1UL << (idx%64);
}

// Same as bitmap_set_single, only difference being that it unsets the target bit.

static inline void bitmap_unset_single(uint64_t idx) {
	bitmap[idx/64] &= ~(1UL << (idx%64));
}

bool bitmap_test_single(uint64_t idx) {
	return (bool) (bitmap[idx/64] & (1UL << (idx%64)));
}

/*
 * bitmap_set: Sets all bits within [from, to) to 1.
 * Note that from is inclusive, to is exclusive.
 * bitmap_unset is analogous to bitmap_set.
 */
static void bitmap_set(uint64_t from, uint64_t to) {
	if (to > max_pfn || from >= to)
		panic("Invalid arguments for bitmap_set");

	for (uint64_t i = from; i < to; i++)
		bitmap_set_single(i);
}

static void bitmap_unset(uint64_t from, uint64_t to) {
	if (to > max_pfn || from >= to)
		panic("Invalid arguments for bitmap_unset");

	for (uint64_t i = from; i < to; i++)
		bitmap_unset_single(i);
}

void bitmap_init(void) {
	find_max_pfn();
	find_bitmap_area((max_pfn+7)/8); // The size of the bitmap should be byte-aligned.
	memset(bitmap, 0xFF, bitmap_sz);

	for (uint64_t i = 0; i < usable_region_count; i++) {
		bitmap_unset(
			usable_regions[i].base >> PAGE_SHIFT,
			(usable_regions[i].base + usable_regions[i].length) >> PAGE_SHIFT
		);
	}

	bitmap_set(bitmap_start >> PAGE_SHIFT, bitmap_end >> PAGE_SHIFT);
}

/*
 * __bitmap_alloc: Helper function of bitmap_alloc.
 * Searches for a contiguous range of pages in [from, to), and sets the bitmap status accordingly.
 * Returns a virtual pointer to the start of the allocated memory.
 */
static void *__bitmap_alloc(uint64_t from, uint64_t to, uint64_t cnt) {
	if (to > max_pfn) to = max_pfn;

bitmap_search:
	while (from+cnt <= to) {
		for (uint64_t i = 0; i < cnt; i++) {
			if (bitmap_test_single(from+i)) {
				from += i+1;
				goto bitmap_search;
			}
		}

		bitmap_set(from, from+cnt);
		bitmap_hint = from+cnt;
		return phys_to_virt(from << PAGE_SHIFT);
	}

	return 0;
}

/*
 * bitmap_alloc: Allocates at least sz bytes of memory.
 * Returns a virtual pointer to the start of the allocated memory.
 * This function will panic if there isn't enough available memory.
 */
void *bitmap_alloc(uint64_t sz) {
	// Count the number of pages to allocate.
	uint64_t cnt = (sz + PAGE_SIZE - 1) / PAGE_SIZE;

	void *res = __bitmap_alloc(bitmap_hint, max_pfn, cnt);			// First, attempt to find space in [bitmap_hint, max_pfn).
	if (!res) res = __bitmap_alloc(0, bitmap_hint+cnt, cnt);		// If the previous attempt fails, try to find space in [0, bitmap_hint+cnt).
	if (!res) panic("Not enough memory available for bitmap_alloc");	// If all fails, panic.

	return res;
}

/*
 * bitmap_free: Return sz bytes of memory to the bitmap allcoator.
 * This function will likely be never called by anything, because most early allocations are permanently used.
 * The base parameter must be virtual.
 * This function will panic if it detects a double-free or an out-of-range-free.
 */
void bitmap_free(void *base, uint64_t sz) {
	uint64_t cnt = (sz + PAGE_SIZE - 1) / PAGE_SIZE;
	uint64_t i = virt_to_phys(base) >> PAGE_SHIFT;
	if (i+cnt > max_pfn) panic("There was an attempt to free an out-of-range page in bitmap_free");

	while (cnt--) {
		if (!bitmap_test_single(i)) panic("Double-free detected in bitmap_free");
		bitmap_unset_single(i++);
	}
}
