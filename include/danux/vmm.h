#ifndef DANUX_VMM_H
#define DANUX_VMM_H

#include <stdint.h>

// 페이지 테이블 엔트리 플래그 (x86_64, 4KB 페이지 전용)
#define VMM_PRESENT	(1UL << 0)
#define VMM_WRITABLE	(1UL << 1)
#define VMM_USER	(1UL << 2)

// 유저 주소 공간 레이아웃. PML4 인덱스 1(0~255 중, vmm_new_address_space가
// 공유하지 않는 하위 절반)에 놓여서 프로세스마다 독립된다.
#define USER_VADDR_BASE	0x0000008000000000UL
#define USER_STACK_TOP	0x0000008040000000UL	// USER_VADDR_BASE + 1GiB

// 부팅 시 CR3에 들어있던 PML4. 모든 새 주소 공간이 커널 영역을 공유하기 위한 기준이 된다.
extern uint64_t *kernel_pml4;

extern void vmm_init(void);
extern void vmm_map(uint64_t *pml4, uint64_t virt, uint64_t phys, uint64_t flags);
extern void vmm_unmap(uint64_t *pml4, uint64_t virt);
extern uint64_t *vmm_new_address_space(void);
extern void vmm_switch_address_space(uint64_t *pml4);

// virt의 매핑 존재 여부만 읽어서 알려준다(테이블을 새로 만들지 않음). 있으면
// 1을 반환하고 flags_out에 PTE 플래그(PRESENT/WRITABLE/USER)를 채운다.
// syscall_validate.c가 유저 포인터 검증에 사용한다.
extern int vmm_lookup(uint64_t *pml4, uint64_t virt, uint64_t *flags_out);

#endif
