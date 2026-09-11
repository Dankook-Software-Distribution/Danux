#ifndef DANUX_PROCESS_H
#define DANUX_PROCESS_H

#include <danux/list.h>
#include <stdint.h>

typedef enum {
	PROC_READY,
	PROC_RUNNING,
	PROC_BLOCKED,
	PROC_ZOMBIE
} proc_state_t;

#define PROC_NAME_MAX	32

typedef struct process {
	uint64_t pid;
	uint64_t *pml4;			// 이 프로세스의 주소 공간 (vmm_new_address_space()가 반환한 가상 포인터)
	uint64_t kernel_stack_top;	// 실행 중일 때 TSS.rsp0/syscall 커널 스택으로 쓰임
	uint64_t user_stack_top;
	uint64_t saved_rsp;		// context_switch()가 쓰는 커널 컨텍스트 스택 포인터
	proc_state_t state;
	char name[PROC_NAME_MAX];
	struct list_head linkage;	// 스케줄러 run queue 연결
} process_t;

// entry_vaddr에 이미 만들어진 유저 코드(code, code_len)를 매핑한 새 주소 공간으로
// 프로세스를 만든다. 실행은 시작하지 않는다 -- scheduler_add() + 스케줄러가 담당.
extern process_t *process_create(const char *name, uint64_t entry_vaddr, const void *code, uint64_t code_len);
extern void process_exit(process_t *proc);

#endif
