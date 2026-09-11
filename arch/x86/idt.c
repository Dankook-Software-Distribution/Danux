#include <danux/gdt.h>
#include <danux/idt.h>
#include <danux/mm.h>
#include <danux/panic.h>
#include <danux/pic.h>
#include <danux/serial.h>
#include <stdint.h>

#define IDT_ENTRIES	256

static struct idt_entry idt[IDT_ENTRIES];
static struct idt_ptr idtp;
static isr_handler_t handlers[IDT_ENTRIES];

extern uint64_t isr_stub_table[48];	// isr_stubs.S

static const char *exception_names[32] = {
	"Divide-by-zero", "Debug", "NMI", "Breakpoint", "Overflow",
	"Bound Range", "Invalid Opcode", "Device N/A", "Double Fault",
	"Coproc Overrun", "Invalid TSS", "Segment N/P", "Stack Fault",
	"GPF", "Page Fault", "Reserved", "x87 FP", "Alignment",
	"Machine Check", "SIMD FP", "Virtualization", "Control Protection",
	"Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
	"Reserved", "Reserved", "Reserved", "Reserved", "Reserved"
};

void idt_set_gate(uint8_t num, uint64_t handler, uint16_t selector, uint8_t ist, uint8_t flags) {
	idt[num].offset_low = handler & 0xFFFF;
	idt[num].offset_mid = (handler >> 16) & 0xFFFF;
	idt[num].offset_high = (uint32_t)(handler >> 32);
	idt[num].selector = selector;
	idt[num].ist = ist & 0x7;
	idt[num].type_attr = flags;
	idt[num].reserved = 0;
}

void idt_init(void) {
	memset(&idt, 0, sizeof(idt));

	// 0x8E = present, DPL0, 64비트 interrupt gate. 예외 + IRQ 0-47 전부 등록.
	for (int i = 0; i < 48; i++)
		idt_set_gate((uint8_t)i, isr_stub_table[i], GDT_KERNEL_CODE, 0, 0x8E);

	idtp.limit = sizeof(idt) - 1;
	idtp.base = (uint64_t)&idt;

	asm volatile ("lidt %0" :: "m"(idtp));
}

void register_interrupt_handler(uint8_t n, isr_handler_t handler) {
	handlers[n] = handler;
}

void isr_dispatch(registers_t *regs) {
	uint64_t n = regs->int_no;

	if (handlers[n]) {
		handlers[n](regs);
	} else if (n < 32) {
		serial_puts("[panic] unhandled exception ");
		serial_putdec(n);
		serial_puts(" (");
		serial_puts(exception_names[n]);
		serial_puts(") err=");
		serial_puthex(regs->err_code);
		serial_puts(" rip=");
		serial_puthex(regs->rip);
		serial_putc('\n');
		panic("unhandled CPU exception");
	}

	// IRQ는 32-47번 벡터에 놓여있음. 핸들러가 다 끝난 뒤에 PIC에 EOI를 보낸다.
	if (n >= 32 && n < 48)
		pic_send_eoi((uint8_t)(n - 32));
}
