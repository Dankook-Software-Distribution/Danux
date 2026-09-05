#include <danux/buddy_allocator.h>
#include <danux/list.h>
#include <danux/mm.h>
#include <danux/page.h>
#include <danux/panic.h>
#include <danux/slab.h>
#include <stdint.h>

// 슬랩이 담당하는 객체 크기 범위: 16B ~ 2KB (2의 거듭제곱 단위)
// 이보다 큰 요청은 버디 할당자에서 페이지 단위로 직접 처리한다.
#define KMALLOC_MIN_SHIFT	4
#define KMALLOC_MAX_SHIFT	11
#define KMALLOC_CACHE_COUNT	(KMALLOC_MAX_SHIFT - KMALLOC_MIN_SHIFT + 1)

struct kmem_cache {
	uint64_t obj_size;
	struct list_head partial_slabs; // 빈 객체가 남아있는 슬랩들의 리스트
};

// 슬랩 하나는 4KB 페이지 한 장이며, 이 헤더가 페이지의 맨 앞에 위치한다.
// 페이지는 한 번 슬랩으로 쓰이면 버디 할당자로 반환하지 않는다.
struct slab {
	struct list_head linkage;	// kmem_cache.partial_slabs에 연결
	struct kmem_cache *cache;
	void *free_list;		// 사용 가능한 객체들의 단일 연결 리스트 (해제된 객체의 앞부분에 next 포인터를 겹쳐 씀)
	uint64_t free_count;
};

static struct kmem_cache kmalloc_caches[KMALLOC_CACHE_COUNT];

void kmalloc_init(void) {
	for (uint8_t i = 0; i < KMALLOC_CACHE_COUNT; i++) {
		kmalloc_caches[i].obj_size = 1UL << (KMALLOC_MIN_SHIFT + i);
		list_head_init(&kmalloc_caches[i].partial_slabs);
	}
}

static struct kmem_cache *cache_for_size(uint64_t size) {
	for (uint8_t shift = KMALLOC_MIN_SHIFT; shift <= KMALLOC_MAX_SHIFT; shift++) {
		if (size <= (1UL << shift))
			return &kmalloc_caches[shift - KMALLOC_MIN_SHIFT];
	}
	panic("kmalloc: size too large for a slab cache");
	return 0;
}

// 새 슬랩 페이지를 버디 할당자에서 받아와 free_list를 구성한다.
static struct slab *slab_create(struct kmem_cache *cache) {
	void *page = page_alloc(PAGE_SIZE);
	uint64_t pfn = virt_to_phys(page) >> PAGE_SHIFT;

	struct slab *slab = (struct slab *)page;
	slab->cache = cache;
	slab->free_list = 0;
	slab->free_count = 0;

	uint8_t *obj_area = (uint8_t *)page + sizeof(struct slab);
	uint64_t obj_capacity = (PAGE_SIZE - sizeof(struct slab)) / cache->obj_size;

	for (uint64_t i = 0; i < obj_capacity; i++) {
		void *obj = obj_area + i * cache->obj_size;
		*(void **)obj = slab->free_list;
		slab->free_list = obj;
	}
	slab->free_count = obj_capacity;

	// 페이지 소유권을 슬랩 할당자로 표시. misc는 버디 할당자가 order를
	// 저장하던 자리지만, 슬랩 페이지는 버디로 되돌아가지 않으므로 그
	// 자리에 대신 slab 구조체 포인터를 저장해도 안전하다.
	page_arr[pfn].flags |= PG_SLAB;
	page_arr[pfn].misc = (uint64_t)slab;

	list_add_next(&cache->partial_slabs, &slab->linkage);

	return slab;
}

void *kmalloc(uint64_t size) {
	if (size == 0)
		return 0;

	// 슬랩 최대 객체 크기를 넘는 요청은 버디 할당자가 페이지 단위로 직접 처리한다.
	if (size > (1UL << KMALLOC_MAX_SHIFT))
		return page_alloc(size);

	struct kmem_cache *cache = cache_for_size(size);

	if (list_empty(&cache->partial_slabs))
		slab_create(cache);

	struct slab *slab = list_entry(cache->partial_slabs.next, struct slab, linkage);

	void *obj = slab->free_list;
	slab->free_list = *(void **)obj;
	slab->free_count--;

	if (slab->free_count == 0)
		list_del(&slab->linkage);

	return obj;
}

void kfree(void *ptr) {
	if (!ptr)
		return;

	uint64_t pfn = virt_to_phys(ptr) >> PAGE_SHIFT;

	// 슬랩 소유가 아니면 kmalloc이 버디 할당자에서 직접 받은 페이지이다.
	if (!(page_arr[pfn].flags & PG_SLAB)) {
		page_free(ptr);
		return;
	}

	struct slab *slab = (struct slab *)page_arr[pfn].misc;
	uint64_t was_full = (slab->free_count == 0);

	*(void **)ptr = slab->free_list;
	slab->free_list = ptr;
	slab->free_count++;

	if (was_full)
		list_add_next(&slab->cache->partial_slabs, &slab->linkage);
}
