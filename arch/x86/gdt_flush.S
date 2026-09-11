.section .text
.code64
.global gdt_flush
.global tss_flush

/* void gdt_flush(uint64_t gdt_ptr_addr) -- rdi = &gdtp */
gdt_flush:
	lgdt (%rdi)

	mov $0x10, %ax		/* GDT_KERNEL_DATA */
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss

	/* 64비트에는 far jmp가 없어서 far return으로 cs를 다시 로드한다 */
	pop %rax
	push $0x08		/* GDT_KERNEL_CODE */
	push %rax
	lretq

/* void tss_flush(uint16_t tss_selector) -- di = selector */
tss_flush:
	mov %di, %ax
	ltr %ax
	ret
