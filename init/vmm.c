#include <danux/buddy_allocator.h>
#include <danux/mm.h>
#include <danux/page.h>
#include <danux/vmm.h>
#include <stdint.h>

// 4KB 페이지 기준, x86_64 4단계 페이징(PML4 -> PDPT -> PD -> PT)의 각 레벨 인덱스.
#define PML4_IDX(v)	(((v) >> 39) & 0x1FF)
#define PDPT_IDX(v)	(((v) >> 30) & 0x1FF)
#define PD_IDX(v)	(((v) >> 21) & 0x1FF)
#define PT_IDX(v)	(((v) >> 12) & 0x1FF)

// 엔트리에서 플래그 비트를 뗀 물리 주소(비트 12~51)만 추출하는 마스크.
#define PTE_ADDR_MASK	0x000FFFFFFFFFF000UL

uint64_t *kernel_pml4;

static uint64_t read_cr3(void) {
	uint64_t val;
	asm volatile("mov %%cr3, %0" : "=r"(val));
	return val;
}

static void write_cr3(uint64_t val) {
	asm volatile("mov %0, %%cr3" : : "r"(val) : "memory");
}

static void invlpg(uint64_t virt) {
	asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

// table[idx]가 가리키는 하위 테이블을 반환한다. 없으면 새로 페이지를 받아 연결한다.
// 상위 레벨 엔트리는 항상 쓰기 가능으로 두고, 실제 접근 권한은 최하위 PT 엔트리에서 결정한다.
static uint64_t *next_table(uint64_t *table, uint64_t idx, uint64_t flags) {
	if (!(table[idx] & VMM_PRESENT)) {
		void *new_table = page_alloc(PAGE_SIZE);
		memset(new_table, 0, PAGE_SIZE);
		table[idx] = virt_to_phys(new_table) | VMM_PRESENT | VMM_WRITABLE | (flags & VMM_USER);
	}
	return phys_to_virt(table[idx] & PTE_ADDR_MASK);
}

// 부트로더(Limine)가 이미 구성해둔 페이지 테이블을 기준 커널 PML4로 삼는다.
void vmm_init(void) {
	kernel_pml4 = phys_to_virt(read_cr3());
}

// virt를 phys에 매핑한다. 중간 테이블이 없으면 새로 만든다.
void vmm_map(uint64_t *pml4, uint64_t virt, uint64_t phys, uint64_t flags) {
	uint64_t *pdpt = next_table(pml4, PML4_IDX(virt), flags);
	uint64_t *pd = next_table(pdpt, PDPT_IDX(virt), flags);
	uint64_t *pt = next_table(pd, PD_IDX(virt), flags);

	pt[PT_IDX(virt)] = (phys & PTE_ADDR_MASK) | flags | VMM_PRESENT;
	invlpg(virt);
}

// virt의 매핑을 해제한다. 중간 테이블이 비어도 회수하지 않는다.
void vmm_unmap(uint64_t *pml4, uint64_t virt) {
	if (!(pml4[PML4_IDX(virt)] & VMM_PRESENT))
		return;
	uint64_t *pdpt = phys_to_virt(pml4[PML4_IDX(virt)] & PTE_ADDR_MASK);

	if (!(pdpt[PDPT_IDX(virt)] & VMM_PRESENT))
		return;
	uint64_t *pd = phys_to_virt(pdpt[PDPT_IDX(virt)] & PTE_ADDR_MASK);

	if (!(pd[PD_IDX(virt)] & VMM_PRESENT))
		return;
	uint64_t *pt = phys_to_virt(pd[PD_IDX(virt)] & PTE_ADDR_MASK);

	pt[PT_IDX(virt)] = 0;
	invlpg(virt);
}

// 새 주소 공간을 만든다. 상위 256개 PML4 엔트리(커널/HHDM 영역)는 kernel_pml4와
// 공유하고, 하위 256개(사용자 영역)는 비워서 반환한다.
uint64_t *vmm_new_address_space(void) {
	uint64_t *pml4 = page_alloc(PAGE_SIZE);
	memset(pml4, 0, PAGE_SIZE);

	for (uint64_t i = 256; i < 512; i++)
		pml4[i] = kernel_pml4[i];

	return pml4;
}

void vmm_switch_address_space(uint64_t *pml4) {
	write_cr3(virt_to_phys(pml4));
}
