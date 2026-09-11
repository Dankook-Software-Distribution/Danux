#include <danux/buddy_allocator.h>
#include <danux/mm.h>
#include <danux/page.h>
#include <danux/process.h>
#include <danux/slab.h>
#include <danux/string.h>
#include <danux/vmm.h>
#include <stdint.h>

// scheduler_switch.S에 구현됨. context_switch()는 스스로 저장해둔 스택을
// 기대하지만, 프로세스가 처음 실행되는 딱 한 번만은 예외다 -- 아래에서
// "이미 저장된 것처럼" 가짜 스택 프레임을 손수 만들어 넣어서, 첫 컨텍스트
// 스위치가 여기로 떨어지게 하고, 여기서 유저 모드로 뛰어든다.
extern void process_start_trampoline(void);

static uint64_t next_pid = 1;

process_t *process_create(const char *name, uint64_t entry_vaddr, const void *code, uint64_t code_len) {
	process_t *proc = kmalloc(sizeof(process_t));
	memset(proc, 0, sizeof(process_t));
	proc->pid = next_pid++;
	proc->state = PROC_READY;
	list_head_init(&proc->linkage);

	uint64_t n = 0;
	while (name[n] && n < PROC_NAME_MAX - 1) { proc->name[n] = name[n]; n++; }
	proc->name[n] = '\0';

	proc->pml4 = vmm_new_address_space();

	// 유저 코드를 페이지 단위로 매핑하고 복사한다.
	for (uint64_t off = 0; off < code_len; off += PAGE_SIZE) {
		void *frame = page_alloc(PAGE_SIZE);
		uint64_t chunk = code_len - off < PAGE_SIZE ? code_len - off : PAGE_SIZE;
		memcpy(frame, (const uint8_t *)code + off, chunk);
		vmm_map(proc->pml4, entry_vaddr + off, virt_to_phys(frame), VMM_WRITABLE | VMM_USER);
	}

	// USER_STACK_TOP 바로 아래 한 페이지짜리 유저 스택.
	void *stack_frame = page_alloc(PAGE_SIZE);
	vmm_map(proc->pml4, USER_STACK_TOP - PAGE_SIZE, virt_to_phys(stack_frame), VMM_WRITABLE | VMM_USER);
	proc->user_stack_top = USER_STACK_TOP;

	// 이 프로세스가 ring0에 있을 때(syscall, 인터럽트) 쓰는 커널 스택.
	// 스케줄될 때 TSS.rsp0도 이 값을 가리키게 된다.
	void *kstack = page_alloc(PAGE_SIZE);
	proc->kernel_stack_top = (uint64_t)kstack + PAGE_SIZE;

	// 첫 context_switch()가 꺼낼 프레임을 손으로 쌓는다: callee-saved
	// 레지스터 + 복귀 주소를, context_switch가 방금 저장한 것처럼. r12/r13은
	// 트램폴린으로 (entry, user_stack)을 넘기는 임시 통로 역할이다 --
	// `ret`으로는 C 인자를 못 넘기기 때문 (scheduler_switch.S 참고).
	uint64_t *sp = (uint64_t *)proc->kernel_stack_top;
	*(--sp) = (uint64_t)process_start_trampoline;	// context_switch의 ret이 돌아갈 주소
	*(--sp) = 0;					// rbx
	*(--sp) = 0;					// rbp
	*(--sp) = entry_vaddr;				// r12 -> 트램폴린 arg0
	*(--sp) = proc->user_stack_top;		// r13 -> 트램폴린 arg1
	*(--sp) = 0;					// r14
	*(--sp) = 0;					// r15
	proc->saved_rsp = (uint64_t)sp;

	return proc;
}

void process_exit(process_t *proc) {
	proc->state = PROC_ZOMBIE;
	// 프레임/페이지테이블 회수는 의도적으로 생략 -- proc->pml4를 순회하며
	// 매핑된 프레임과 페이지테이블 프레임 자체를 page_free()로 돌려줘야
	// 완전해지지만, "일단 돌아가는" PoC 범위 밖으로 남겨둔다.
}
