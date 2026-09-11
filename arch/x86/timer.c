#include <danux/idt.h>
#include <danux/pic.h>
#include <danux/timer.h>
#include <io.h>
#include <stdint.h>

#define PIT_CHANNEL0	0x40
#define PIT_COMMAND	0x43
#define PIT_BASE_HZ	1193182

volatile uint64_t timer_ticks = 0;

void timer_handler(registers_t *regs) {
	(void)regs;
	timer_ticks++;
	// scheduler_init()이 이 벡터의 핸들러를 scheduler_tick()으로 덮어씀.
	// 그쪽에서도 tick 카운트를 유지하려고 이 함수를 다시 호출해준다.
}

void timer_init(uint32_t freq_hz) {
	uint32_t divisor = PIT_BASE_HZ / freq_hz;

	outb(0x36, PIT_COMMAND);	// channel 0, lo/hi, mode 3 (사각파)
	outb(divisor & 0xFF, PIT_CHANNEL0);
	outb((divisor >> 8) & 0xFF, PIT_CHANNEL0);

	register_interrupt_handler(32, timer_handler);	// IRQ0 -> 벡터 32

	// pic_remap()은 기존 마스크를 그대로 보존하는데, 펌웨어/부트로더가
	// IRQ0을 masked 상태로 넘겨주면 타이머가 영원히 안 울린다.
	pic_clear_mask(0);
}
