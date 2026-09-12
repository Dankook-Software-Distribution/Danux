#include <danux/bitmap.h>
#include <danux/buddy_allocator.h>
#include <danux/list.h>
#include <danux/mm.h>
#include <danux/page.h>
#include <danux/panic.h>
#include <stdbool.h>
#include <stdint.h>

// 버디 할당자 구조체 정의
struct buddy_system {
	struct list_head free_list[MAX_ORDER + 1];	// 0 ~ 10 (4KB ~ 4MB)
	uint64_t total_pages;
	uint64_t free_pages;
};

// 외부에서 직접적으로는 사용할 수 없게 처리
static buddy_system g_buddy_system;

/******************
 * 함수 정의 시작 *
 ******************/
static void split_free_page(uint8_t order) {
	struct list_head *node = g_buddy_system.free_list[order].next;
	struct page *page_addr = list_entry(node, struct page, linkage);

	uint64_t left_pfn = page_to_pfn(page_addr);
	uint64_t right_pfn = left_pfn + (1UL << (order - 1));

	// node(= left_pfn의 linkage)를 현재 order의 free_list에서 먼저 떼어내야 한다.
	// list_del은 node->prev/next를 보고 원래 리스트에서 빼는데, 만약 아래
	// list_add_next를 먼저 해버리면 그 시점에 이미 node가 order-1 리스트로
	// 옮겨진 뒤라서, list_del이 방금 넣은 order-1 리스트에서 다시 빼버리게
	// 된다 -- left_pfn 페이지가 어느 free_list에도 없는 채로 유실되고,
	// order 레벨의 head는 끊어진 node를 계속 가리키는 상태로 남는다.
	list_del(node);

	// 두 조각 모두 이제 order-1짜리 free 블록의 머리다.
	// misc에 order를 남겨야 page_free의 합치기 조건이 이걸 확인할 수 있다.
	page_arr[left_pfn].flags |= PG_BUDDY;
	page_arr[left_pfn].misc = order - 1;
	page_arr[right_pfn].flags |= PG_BUDDY;
	page_arr[right_pfn].misc = order - 1;

	// 쪼갠 free_page를 현재 order의 하위 free_list에 넣기
	list_add_next(&g_buddy_system.free_list[order - 1], &page_arr[left_pfn].linkage);
	list_add_next(&g_buddy_system.free_list[order - 1], &page_arr[right_pfn].linkage);
}

static void *get_free_page(uint8_t order) {
	// list_head 구조체를 구함
	struct list_head *node = g_buddy_system.free_list[order].next;

	// list_entry는 매크로이며 linkage로 정의된 node(list_head)가 속해있는 page 구조체의 주소를 반환함
	// 그걸 page_to_pfn을 통해 page 구조체의 주소를 pfn으로 변환함
	uint64_t pfn = page_to_pfn(list_entry(node, struct page, linkage));

	page_arr[pfn].flags &= ~PG_BUDDY;
	page_arr[pfn].misc = (uint64_t)order;

	// 할당한 페이지 삭제
	list_del(node);

	// 물리주소 pfn << PAGE_SHIFT를 가상 주소로 변환
	return phys_to_virt(pfn << PAGE_SHIFT);
}

static inline uint64_t buddy_pfn_of(uint64_t pfn, uint8_t order) {
	return pfn ^ (1UL << order);
}

void *page_alloc(uint64_t byte_size) {
	// order 0부터 요청한 바이트를 다 담을 수 있는 order 계산
	uint8_t order = 0;
	uint64_t page_cnt = (byte_size + PAGE_SIZE - 1) / PAGE_SIZE;
	while ((1UL << order) < page_cnt)
		order++;
	if (order > MAX_ORDER)
		panic("Order is too large!");

	// 사용자가 실제로 원하는 페이지 크기: orig_order
	uint8_t orig_order = order;

	// 원하는 페이지가 없으면 상위 order에서 탐색
	while (order <= MAX_ORDER && list_empty(&g_buddy_system.free_list[order]))
		order++;
	if (order > MAX_ORDER)
		panic("Not Enough Page Order");

	// 쪼개줄 수 있는 order에서 요청한 orig_order까지
	// 페이지를 쪼개고 넣으면서 내려옴
	for (uint8_t i = order; i > orig_order; i--) {
		split_free_page(i);
	}

	g_buddy_system.free_pages -= (1UL << orig_order);

	return get_free_page(orig_order);
}

void page_free(void *addr) {
	// 가상 주소로 받은 addr로 PFN를 구해야함 -> virt_to_phys() == PFN
	uint64_t pfn = virt_to_phys((void *)addr) >> PAGE_SHIFT;
	uint8_t order = (uint8_t)page_arr[pfn].misc;

	while (order < MAX_ORDER) {
		uint64_t buddy_pfn = buddy_pfn_of(pfn, order);

		// page_arr 밖으로 나가면 합칠 상대가 없다.
		if (buddy_pfn >= max_pfn)
			break;

		/*
		 * 버디가 "같은 order로 통째로" 비어 있을 때만 합칠 수 있다.
		 * PG_BUDDY만 보고 합치면, 버디가 더 작은 블록의 머리일 뿐인데도
		 * 그 뒤쪽 할당된 페이지까지 같이 삼켜버린다.
		 */
		if (!(page_arr[buddy_pfn].flags & PG_BUDDY))
			break;
		if ((uint8_t)page_arr[buddy_pfn].misc != order)
			break;

		// 합치기 전 buddy를 free_list에서 삭제
		list_del(&page_arr[buddy_pfn].linkage);
		page_arr[buddy_pfn].flags &= ~PG_BUDDY;

		// 두 페이지 중 더 낮은 번호의 페이지가 합쳐진 블록의 머리가 된다.
		if (buddy_pfn < pfn)
			pfn = buddy_pfn;
		order++;
	}

	/*
	 * 머리 표시는 합치기가 끝난 뒤에 해야 한다.
	 *
	 * 예전에는 루프 전에 "넘겨받은 pfn"에만 PG_BUDDY를 달았다. 그런데
	 * 합쳐지면 머리가 버디 쪽으로 넘어갈 수 있고, 루프는 그 버디의
	 * PG_BUDDY를 지운다. 그래서 free_list에 들어간 블록의 머리에 PG_BUDDY가
	 * 없는 상태가 만들어졌고, 이후 어떤 합치기도 그 블록을 알아보지 못했다.
	 */
	page_arr[pfn].flags = PG_BUDDY;
	page_arr[pfn].misc = order;

	list_add_next(&g_buddy_system.free_list[order], &page_arr[pfn].linkage);

	g_buddy_system.free_pages += (1UL << order);
}

static void buddy_list_init() {
	for (uint8_t i = 0; i < MAX_ORDER + 1; i++) {
		list_head_init(&g_buddy_system.free_list[i]);
	}
	g_buddy_system.total_pages = max_pfn;
	g_buddy_system.free_pages = 0;
}

void buddy_init() {
	buddy_list_init();

	for (uint64_t i = 0; i < max_pfn; i++) {
		// 비트맵이 0이면 사용 가능
		// -> bitmap_test_single은 사용할 수 있으면 False 반환
		if (!bitmap_test_single(i)) {
			page_free(phys_to_virt((uint64_t)(i << PAGE_SHIFT)));
		}
	}
}
