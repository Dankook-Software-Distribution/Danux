/* syscall_entry.S -- SYSCALL/SYSRET 고속 경로, IA32_LSTAR에 설치된다.
 *
 * SYSCALL 실행 시 CPU가 RIP<-RCX(복귀 주소), RFLAGS<-R11, CS/SS<-STAR MSR을
 * 자동으로 채워주지만 스택은 안 바꿔준다 -- 그건 gs 기반 percpu 스크래치
 * 영역(swapgs)을 통해 우리가 직접 한다. 실제 커널들도 이렇게 한다.
 *
 * percpu 레이아웃 (syscall.c 참고):
 *   gs:0  kernel_rsp   (고정, syscall_init()에서 한 번 설정)
 *   gs:8  user_rsp     (매 syscall마다 저장/복원되는 스크래치)
 */
.section .text
.code64
.global syscall_entry
.extern syscall_dispatch

syscall_entry:
	swapgs
	mov %rsp, %gs:8
	mov %gs:0, %rsp

	push %rax
	push %rbx
	push %rcx		/* == user rip, SYSCALL이 rcx에 저장해둠 */
	push %rdx
	push %rsi
	push %rdi
	push %rbp
	push %r8
	push %r9
	push %r10		/* 4번째 syscall 인자 -- rcx는 이미 써서 못 씀 */
	push %r11		/* == user rflags, SYSCALL이 r11에 저장해둠 */
	push %r12
	push %r13
	push %r14
	push %r15

	push %rcx		/* user_rip 필드 */
	push %r11		/* user_rflags 필드 */
	push %gs:8		/* user_rsp 필드 */

	mov %rsp, %rdi
	call syscall_dispatch	/* syscall_dispatch()가 frame->rax에 직접 결과를 써준다 */

	pop %rax		/* user_rsp -> 임시 보관 */
	mov %rax, %gs:8
	pop %r11		/* user_rflags (아래 GP r11과 값은 같음, 중복) */
	pop %rcx		/* user_rip (아래 GP rcx와 값은 같음, 중복) */

	pop %r15
	pop %r14
	pop %r13
	pop %r12
	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rbp
	pop %rdi
	pop %rsi
	pop %rdx
	pop %rcx
	pop %rbx
	pop %rax

	mov %gs:8, %rsp
	swapgs
	sysretq
