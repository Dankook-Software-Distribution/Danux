#include <danux/gdt.h>
#include <danux/scheduler.h>
#include <danux/serial.h>
#include <danux/syscall.h>
#include <danux/syscall_validate.h>
#include <stdint.h>

#define MSR_EFER		0xC0000080
#define MSR_STAR		0xC0000081
#define MSR_LSTAR		0xC0000082
#define MSR_FMASK		0xC0000084
#define MSR_GS_BASE		0xC0000101
#define MSR_KERNEL_GS_BASE	0xC0000102

#define EFER_SCE	(1 << 0)

// syscall_entry.S가 %gs:0 / %gs:8로 읽는 percpu 영역. offset0=kernel_rsp
// (TSS/스케줄러 세팅 이후 고정), offset8=user_rsp(매 syscall마다 스크래치).
struct percpu {
	uint64_t kernel_rsp;
	uint64_t user_rsp;
};
static struct percpu percpu;

static inline void wrmsr(uint32_t msr, uint64_t val) {
	uint32_t lo = (uint32_t)val;
	uint32_t hi = (uint32_t)(val >> 32);
	asm volatile ("wrmsr" :: "c"(msr), "a"(lo), "d"(hi));
}

static inline uint64_t rdmsr(uint32_t msr) {
	uint32_t lo, hi;
	asm volatile ("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
	return ((uint64_t)hi << 32) | lo;
}

void syscall_init(void) {
	wrmsr(MSR_EFER, rdmsr(MSR_EFER) | EFER_SCE);

	// STAR[47:32] = SYSCALL용 base: CS<-base, SS<-base+8.
	//   GDT_KERNEL_CODE(0x08) -> CS=0x08, SS=0x08+8=0x10=GDT_KERNEL_DATA. OK.
	// STAR[63:48] = SYSRET용 base: CS<-(base+16)|3, SS<-(base+8)|3.
	//   GDT_KERNEL_DATA(0x10)를 base로 쓰면 CS=(0x10+16)|3=0x23=GDT_USER_CODE,
	//   SS=(0x10+8)|3=0x1B=GDT_USER_DATA -- gdt.h가 user data(0x18)를 user
	//   code(0x20) 바로 앞에 두는 이유가 바로 이거다. SYSRET은 이렇게 인접한
	//   셀렉터 쌍만 만들어낼 수 있다.
	uint64_t star = ((uint64_t)GDT_KERNEL_DATA << 48) | ((uint64_t)GDT_KERNEL_CODE << 32);
	wrmsr(MSR_STAR, star);
	wrmsr(MSR_LSTAR, (uint64_t)syscall_entry);
	wrmsr(MSR_FMASK, 0x200);	// 진입 시 IF 클리어 -- syscall 도중 중첩 인터럽트 방지

	wrmsr(MSR_GS_BASE, 0);				// 유저 모드 GS, 지금은 미사용
	wrmsr(MSR_KERNEL_GS_BASE, (uint64_t)&percpu);
}

void syscall_set_kernel_stack(uint64_t rsp0) {
	percpu.kernel_rsp = rsp0;
}

static uint64_t sys_write(uint64_t fd, uint64_t buf, uint64_t len) {
	(void)fd;
	if (!validate_user_range(current_process, (const void *)buf, len, 0))
		return (uint64_t)-1;	// -EFAULT

	const char *s = (const char *)buf;
	for (uint64_t i = 0; i < len; i++)
		serial_putc(s[i]);
	return len;
}

uint64_t syscall_dispatch(syscall_frame_t *frame) {
	uint64_t result;

	switch (frame->rax) {
	case SYS_WRITE:
		result = sys_write(frame->rdi, frame->rsi, frame->rdx);
		break;
	case SYS_GETPID:
		result = current_process->pid;
		break;
	case SYS_EXIT:
		process_exit(current_process);
		/*
		 * syscall_entry가 진입할 때 건 swapgs를 여기서 직접 되돌린다.
		 *
		 * 정상 경로라면 syscall_entry의 마지막 swapgs가 짝을 맞춰주지만,
		 * 아래 hlt 루프는 그 경로로 절대 돌아가지 않는다. 그대로 두면
		 * GS_BASE=&percpu, KERNEL_GS_BASE=0인 상태로 굳어버리고,
		 * 스케줄러가 다음 프로세스로 넘어간 뒤 그 프로세스가 처음
		 * syscall을 하는 순간 swapgs가 GS_BASE를 0으로 만들어
		 * `mov %rsp, %gs:8`이 주소 8에 쓰다가 #PF로 죽는다.
		 *
		 * enter_usermode는 `mov %ax, %gs`로 GS_BASE만 0으로 만들 뿐
		 * KERNEL_GS_BASE는 건드리지 않으므로 이 불균형을 복구해주지 못한다.
		 */
		asm volatile ("swapgs");

		// 죽은 프로세스의 유저 코드로 다시 sysretq하지 않는다. 대신 이 커널
		// 스택을 인터럽트 가능한 hlt 루프에 묶어둔다 -- 다음 타이머 틱이
		// (유저 모드가 아니라) 여기로 떨어져서 이 프로세스가 ZOMBIE임을 보고
		// pick_next()가 이후로 영원히 건너뛰게 된다.
		asm volatile ("sti");
		for (;;) asm volatile ("hlt");
	default:
		serial_puts("[syscall] unknown number ");
		serial_putdec(frame->rax);
		serial_putc('\n');
		result = (uint64_t)-1;
		break;
	}

	frame->rax = result;
	return result;
}
