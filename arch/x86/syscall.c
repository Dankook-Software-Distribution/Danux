#include <danux/gdt.h>
#include <danux/serial.h>
#include <danux/syscall.h>
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

// 프로세스별 주소 공간 검증이 아직 없어서 buf를 검증 없이 그대로 읽는다.
static uint64_t sys_write(uint64_t fd, uint64_t buf, uint64_t len) {
	(void)fd;
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
		// 아직 현재 프로세스를 추적하는 스케줄러가 없다.
		result = 0;
		break;
	case SYS_EXIT:
		// 프로세스 종료 처리가 아직 없다. 최소한 유저 코드로 sysretq해서
		// 계속 도는 일은 없도록 여기서 멈춘다.
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
