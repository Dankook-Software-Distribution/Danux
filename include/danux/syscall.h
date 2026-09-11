#ifndef DANUX_SYSCALL_H
#define DANUX_SYSCALL_H

#include <stdint.h>

#define SYS_WRITE	1
#define SYS_READ	2
#define SYS_EXIT	3
#define SYS_GETPID	4

// syscall_entry.S가 syscall_dispatch()를 부르기 전에 저장해두는 레지스터 프레임.
// 필드 순서는 syscall_entry.S의 push 순서와 정확히 맞아야 한다: 거기서 마지막에
// push하는 값(user_rsp)이 가장 낮은 주소, 즉 struct offset 0에 온다 (C로 넘기는
// 포인터가 push가 다 끝난 뒤의 %rsp이므로).
typedef struct syscall_frame {
	uint64_t user_rsp;
	uint64_t user_rflags;
	uint64_t user_rip;	// SYSCALL이 rcx에 저장한 걸 여기로 옮김
	uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
	uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
} syscall_frame_t;

extern void syscall_init(void);
extern void syscall_set_kernel_stack(uint64_t rsp0);
extern uint64_t syscall_dispatch(syscall_frame_t *frame);

// syscall_entry.S에 구현됨. LSTAR MSR에 설치된다.
extern void syscall_entry(void);

#endif
