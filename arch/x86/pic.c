#include <danux/mm.h>
#include <danux/pic.h>
#include <danux/vmm.h>
#include <io.h>
#include <stdint.h>

#define PIC1		0x20
#define PIC2		0xA0
#define PIC1_CMD	PIC1
#define PIC1_DATA	(PIC1 + 1)
#define PIC2_CMD	PIC2
#define PIC2_DATA	(PIC2 + 1)

#define ICW1_INIT	0x10
#define ICW1_ICW4	0x01
#define ICW4_8086	0x01
#define PIC_EOI		0x20

#define MSR_APIC_BASE	0x1B
#define LAPIC_REG_SVR	0x0F0	// Spurious Interrupt Vector Register
#define LAPIC_REG_LINT0	0x350	// LVT LINT0 entry

static inline void io_wait(void) {
	outb(0, 0x80);
}

static inline uint64_t rdmsr(uint32_t msr) {
	uint32_t lo, hi;
	asm volatile ("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
	return ((uint64_t)hi << 32) | lo;
}

// 로컬 APIC의 LINT0 핀은 레거시 8259 PIC의 virtual-wire INTR 출력을 받는
// 자리인데, 리셋 직후에는 masked 상태다. 진짜 BIOS는 부팅 과정에서 이걸
// ExtINT 모드로 열어주지만 Limine은 그 일을 하지 않으므로, 여기서 직접
// 열어주지 않으면 8259가 IRQ를 올려도(IRR에 잡혀도) CPU가 영원히 못 받는다.
static void lapic_unmask_lint0(void) {
	uint64_t apic_base = rdmsr(MSR_APIC_BASE) & 0xFFFFF000UL;
	volatile uint32_t *lapic = phys_to_virt(apic_base);

	// Limine의 HHDM은 LAPIC MMIO 영역(0xFEE00000)까지는 덮지 않으므로
	// 직접 매핑해야 한다.
	vmm_map(kernel_pml4, (uint64_t)lapic, apic_base, VMM_WRITABLE);

	lapic[LAPIC_REG_SVR / 4] |= 0x1FF;	// APIC software enable + spurious vector 0xFF
	lapic[LAPIC_REG_LINT0 / 4] = 0x700;	// ExtINT, unmasked
}

// 8259 PIC 한 쌍을 재초기화해서 IRQ0-15가 CPU 예외(0-31)와 겹치지 않는
// offset1..offset1+7 / offset2..offset2+7 벡터로 오도록 만들고,
// 로컬 APIC LINT0도 함께 열어서 실제로 CPU까지 전달되게 한다.
void pic_remap(int offset1, int offset2) {
	uint8_t mask1 = inb(PIC1_DATA);
	uint8_t mask2 = inb(PIC2_DATA);

	outb(ICW1_INIT | ICW1_ICW4, PIC1_CMD); io_wait();
	outb(ICW1_INIT | ICW1_ICW4, PIC2_CMD); io_wait();

	outb((uint8_t)offset1, PIC1_DATA); io_wait();
	outb((uint8_t)offset2, PIC2_DATA); io_wait();

	outb(4, PIC1_DATA); io_wait();	// 마스터에게: 슬레이브가 IRQ2에 있음
	outb(2, PIC2_DATA); io_wait();	// 슬레이브에게: 자신의 cascade identity

	outb(ICW4_8086, PIC1_DATA); io_wait();
	outb(ICW4_8086, PIC2_DATA); io_wait();

	outb(mask1, PIC1_DATA);
	outb(mask2, PIC2_DATA);

	lapic_unmask_lint0();
}

void pic_send_eoi(uint8_t irq) {
	if (irq >= 8) outb(PIC_EOI, PIC2_CMD);
	outb(PIC_EOI, PIC1_CMD);
}

void pic_set_mask(uint8_t irq) {
	uint16_t port = irq < 8 ? PIC1_DATA : PIC2_DATA;
	uint8_t bit = irq < 8 ? irq : (uint8_t)(irq - 8);
	outb((uint8_t)(inb(port) | (1 << bit)), port);
}

void pic_clear_mask(uint8_t irq) {
	uint16_t port = irq < 8 ? PIC1_DATA : PIC2_DATA;
	uint8_t bit = irq < 8 ? irq : (uint8_t)(irq - 8);
	outb((uint8_t)(inb(port) & ~(1 << bit)), port);
}
