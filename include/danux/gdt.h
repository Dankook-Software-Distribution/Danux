#ifndef DANUX_GDT_H
#define DANUX_GDT_H

#include <stdint.h>

// 셀렉터 값은 gdt_init()이 만드는 엔트리 순서에 고정되어 있음.
// user data(0x18)는 반드시 user code(0x20) 바로 앞에 와야 한다 --
// SYSRET이 STAR의 base 하나로부터 SS=(base+8)|3, CS=(base+16)|3를
// 동시에 유도하는데, 이 순서가 아니면 selector가 어긋난다. (syscall.c 참고)
#define GDT_KERNEL_CODE	0x08
#define GDT_KERNEL_DATA	0x10
#define GDT_USER_DATA	(0x18 | 3)	// RPL 3
#define GDT_USER_CODE	(0x20 | 3)
#define GDT_TSS_SEL	0x28

struct gdt_entry {
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t base_mid;
	uint8_t access;
	uint8_t granularity;
	uint8_t base_high;
} __attribute__((packed));

// long mode의 TSS descriptor는 16바이트(GDT 슬롯 2칸)를 차지한다.
struct gdt_tss_entry {
	uint16_t length;
	uint16_t base_low;
	uint8_t base_mid;
	uint8_t flags1;
	uint8_t flags2;
	uint8_t base_high;
	uint32_t base_upper;
	uint32_t reserved;
} __attribute__((packed));

struct gdt_ptr {
	uint16_t limit;
	uint64_t base;
} __attribute__((packed));

struct tss_entry {
	uint32_t reserved0;
	uint64_t rsp0;	// ring3 -> ring0 전환 시 CPU가 자동으로 불러오는 커널 스택
	uint64_t rsp1;
	uint64_t rsp2;
	uint64_t reserved1;
	uint64_t ist1;	// Interrupt Stack Table (#DF, NMI 등에 사용, 지금은 미사용)
	uint64_t ist2;
	uint64_t ist3;
	uint64_t ist4;
	uint64_t ist5;
	uint64_t ist6;
	uint64_t ist7;
	uint64_t reserved2;
	uint16_t reserved3;
	uint16_t iomap_base;
} __attribute__((packed));

extern void gdt_init(void);
extern void tss_set_kernel_stack(uint64_t rsp0);

// gdt_flush.S에 구현됨
extern void gdt_flush(uint64_t gdt_ptr_addr);
extern void tss_flush(uint16_t tss_selector);

#endif
