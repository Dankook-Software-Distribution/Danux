/* usermode_enter.S -- 가짜 인터럽트 프레임을 쌓고 iretq로 ring3에 진입한다.
 * void enter_usermode(uint64_t entry, uint64_t user_stack); -- entry는 rdi, user_stack은 rsi */
.section .text
.code64
.global enter_usermode

#define USER_DATA_SEL 0x1B	/* GDT_USER_DATA (0x18) | RPL 3 */
#define USER_CODE_SEL 0x23	/* GDT_USER_CODE (0x20) | RPL 3 */

enter_usermode:
	mov $USER_DATA_SEL, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	/* %ss는 아래 iretq 자체가 로드하므로 여기서 건드리지 않는다 */

	push $USER_DATA_SEL	/* ss */
	push %rsi		/* rsp (user_stack) */
	pushf			/* rflags */
	pop %rax
	or  $0x200, %rax	/* IF=1 보장, 유저 모드에서도 인터럽트는 켜둔다 */
	push %rax
	push $USER_CODE_SEL	/* cs */
	push %rdi		/* rip (entry) */
	iretq
