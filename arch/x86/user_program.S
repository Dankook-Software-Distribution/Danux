/* user_program.S -- 데모용 최소 ring3 페이로드: write() 하고 exit()를 반복.
 * rip-relative 주소 지정만 쓰도록 작성했다 (근접/로컬 분기만 사용) -- process.c가
 * 이 바이트 범위를 그대로 프로세스 주소 공간의 USER_VADDR_BASE로 복사하는데,
 * 커널 이미지 안에서 링크된 주소와는 전혀 다른 위치이기 때문이다.
 * syscall 번호(1=write, 3=exit)는 include/danux/syscall.h와 맞아야 한다. */
.section .rodata
.align 16
.global user_program_start
.global user_program_end

user_program_start:
	mov $1, %rax		/* SYS_WRITE */
	mov $1, %rdi		/* fd, sys_write()에서는 안 씀 */
	lea msg(%rip), %rsi
	mov $msg_len, %rdx
	syscall
.Lspin:
	mov $3, %rax		/* SYS_EXIT */
	syscall
	jmp .Lspin
msg:
	.ascii "hello from userspace\n"
msg_len = . - msg
user_program_end:
