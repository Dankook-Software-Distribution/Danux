#include <danux/mm.h>
#include <limine.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct usable_region usable_regions[MAX_USABLE_REGIONS];
uint64_t usable_region_count;
uint64_t hhdm_offset;

extern void bitmap_init(void);

/*
 * __attribute__((used, section(".limine_requests")))
 * volatile 구조
 * ->	Limine과 통신하는 요청 구조체가 컴파일러 최적화에도 살아남아
 *  	올바른 위치에 놓이도록 보장하는 구조
 */

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(6);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
	.id = LIMINE_MEMMAP_REQUEST_ID,
	.revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
	.id = LIMINE_FRAMEBUFFER_REQUEST_ID,
	.revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
	.id = LIMINE_HHDM_REQUEST_ID,
	.revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

// "HCF: Halt and Catch Fire"
// CPU를 영원히 정지시킨다
static void hcf(void) {
	for (;;)
		asm ("hlt");
}

// The following will be our kernel's entry point.
void kmain(void) {
	/*
	 * 전역변수 설정에서 limine_base_revision[2]가 LIMINE_BASE_REVISION() 매크로에 의해 6으로 설정됨
	 * 그리고 LIMINE_BASE_REVISION_SUPPORTED 매크로는 limine_base_revision[2] == 0을 수행함
	 * 이때 두 값이 다르니까 false가 나온다고 헷갈릴 수 있음
	 * 하지만 이건 부트로더가 건드리기 전이어서 그렇게 보이는 거임
	 * 실제로 QEMU에서 Limine으로 부팅했을 때  Limine이 revision 6을 지원하면 [2]를 0으로 바꿔줌
	 *
	 * 즉, 이 코드는 부트로더가 우리 프로토콜 버전을 이해하는지 확인하는 로직임
	 */
	if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
		hcf();
	}
	
	/*
	 * Limine이 응답 구조체를 메모리에 만들어놓고 그 주소를 response에 직접 써줌
	 * 그래서 kmain이 실행될 때는 이미 response 포인터가 채워져 있음
	 * 그걸 확인하는 로직
	 */
	if (memmap_request.response == NULL) {
		hcf();
	}

	/*
	 * Limine가 매핑한 HHDM 시작 주소를 받아올 수 있는지 확인
	 */
	if (hhdm_request.response == NULL) {
		hcf();
	}

	/*
	 * framebuffer도 비슷함
	 * 대신 framebuffer의 경우, framebuffer가 0개면 그릴 화면이 없기 때문에 개수도 확인함
	 */
	if (framebuffer_request.response == NULL
	|| framebuffer_request.response->framebuffer_count < 1) {
		hcf();
	}

	// limine_memmap_response 구조체의 경우, 내부 필드를 Limine가 다 채워줌 (revision, entry_count, **entries)
	struct limine_memmap_response *response = memmap_request.response;
	for (uint64_t i = 0; i < response->entry_count; i++) {
		struct limine_memmap_entry *entry = response->entries[i];
		if (entry == NULL) continue;

		// USABLE TYPE(0)이라면 usable_regions 배열에 추가
		// mini-dOS와 달리 base, length를 사용 (!= start, end)
		if (entry->type == LIMINE_MEMMAP_USABLE) {
			if (usable_region_count >= MAX_USABLE_REGIONS) {
				hcf();
			}
			usable_regions[usable_region_count].base = entry->base;
			usable_regions[usable_region_count].length = entry->length;
			usable_region_count++;
		}
	}

	hhdm_offset = hhdm_request.response->offset;
	bitmap_init();

	// Fetch the first framebuffer.
	struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];

	// Print a nice pattern to screen as an example.
	// Note: we assume the framebuffer model is RGB with 32-bit pixels.
	volatile uint32_t *fb_ptr = framebuffer->address;
	for (size_t y = 0; y < framebuffer->height; y++) {
		for (size_t x = 0; x < framebuffer->width; x++) {
			uint32_t nX = x * 255 / framebuffer->width;
			uint32_t nY = y * 255 / framebuffer->height;
			fb_ptr[y * (framebuffer->pitch / 4) + x] = (nY << 8) | nX;
		}
	}

	// We're done, just hang...
	hcf();
}
