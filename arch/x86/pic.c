#include <danux/pic.h>
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

static inline void io_wait(void) {
	outb(0, 0x80);
}

// 8259 PIC 한 쌍을 재초기화해서 IRQ0-15가 CPU 예외(0-31)와 겹치지 않는
// offset1..offset1+7 / offset2..offset2+7 벡터로 오도록 만든다.
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
