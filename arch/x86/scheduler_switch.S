/* scheduler_switch.S -- 타이머 IRQ가 몰아주는, 겉보기엔 협조적인 스위치.
 *
 * context_switch()는 callee-saved 레지스터와 RSP만 저장/복원한다.
 * 인터럽트당한 프로세스의 나머지 전체 레지스터 상태는 scheduler_tick()이
 * 실행되기 전에 이미 isr_stubs.S가 그 프로세스 자신의 커널 스택에 쌓아둔
 * 상태다. RSP만 바꾸고 `ret`하면, 나중에 scheduler_tick -> isr_dispatch ->
 * isr_common의 뒷부분 -> iretq로 되감겨서 그 프로세스의 유저 모드 실행이
 * 실제로 재개된다. 새 프로세스가 처음 스위치될 때 이 스택 모양을 흉내내는
 * 방법은 process.c를 참고.
 */
.section .text
.code64
.global context_switch
.global process_start_trampoline
.extern enter_usermode

/* void context_switch(uint64_t *old_rsp_store, uint64_t new_rsp); -- old_rsp_store는 rdi, new_rsp는 rsi */
context_switch:
	push %rbx
	push %rbp
	push %r12
	push %r13
	push %r14
	push %r15

	mov %rsp, (%rdi)
	mov %rsi, %rsp

	pop %r15
	pop %r14
	pop %r13
	pop %r12
	pop %rbp
	pop %rbx
	ret

/* 프로세스가 처음으로 context_switch()되어 들어올 때 떨어지는 착지 지점.
 * process.c가 가짜 저장 프레임에 (entry, user_stack)을 r12/r13에 미리
 * 넣어뒀다 -- `ret`으로는 C 인자를 못 넘기기 때문. */
process_start_trampoline:
	mov %r12, %rdi
	mov %r13, %rsi
	call enter_usermode
	/* enter_usermode()는 noreturn(iretq로 ring3 진입)이라 여기 도달 안 함 */
1:	hlt
	jmp 1b
