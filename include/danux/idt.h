#ifndef DANUX_IDT_H
#define DANUX_IDT_H

#include <stdint.h>

struct idt_entry {
	uint16_t offset_low;
	uint16_t selector;
	uint8_t ist;		// 하위 3비트가 TSS.ist[1..7]을 가리킴, 0이면 미사용
	uint8_t type_attr;
	uint16_t offset_mid;
	uint32_t offset_high;
	uint32_t reserved;
} __attribute__((packed));

struct idt_ptr {
	uint16_t limit;
	uint64_t base;
} __attribute__((packed));

// isr_stubs.S가 C로 넘어오기 전에 스택에 쌓아두는 레지스터 스냅샷.
typedef struct registers {
	uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
	uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
	uint64_t int_no, err_code;
	uint64_t rip, cs, rflags, rsp, ss;
} registers_t;

typedef void (*isr_handler_t)(registers_t *);

extern void idt_init(void);
extern void idt_set_gate(uint8_t num, uint64_t handler, uint16_t selector, uint8_t ist, uint8_t flags);
extern void register_interrupt_handler(uint8_t n, isr_handler_t handler);

// isr_stubs.S에서 모든 벡터마다 호출함
extern void isr_dispatch(registers_t *regs);

#endif
