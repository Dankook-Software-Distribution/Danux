#include <danux/gdt.h>
#include <danux/mm.h>
#include <stdint.h>

// null, kcode, kdata, udata, ucode, tss(슬롯 2개)
#define GDT_ENTRY_COUNT	7

static struct gdt_entry gdt[GDT_ENTRY_COUNT];
static struct gdt_ptr gdtp;
static struct tss_entry tss;

// 특권 레벨 전환 시 쓸 커널 스택. process.c/scheduler.c가 컨텍스트 스위치마다
// tss_set_kernel_stack()으로 프로세스별 커널 스택으로 교체한다.
static uint8_t kernel_stack[16384] __attribute__((aligned(16)));

static void gdt_set_entry(int idx, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
	gdt[idx].base_low = base & 0xFFFF;
	gdt[idx].base_mid = (base >> 16) & 0xFF;
	gdt[idx].base_high = (base >> 24) & 0xFF;
	gdt[idx].limit_low = limit & 0xFFFF;
	gdt[idx].granularity = gran & 0xF0;
	gdt[idx].granularity |= (limit >> 16) & 0x0F;
	gdt[idx].access = access;
}

static void gdt_set_tss(int idx, uint64_t base, uint32_t limit) {
	struct gdt_tss_entry *t = (struct gdt_tss_entry *)&gdt[idx];
	t->length = limit & 0xFFFF;
	t->base_low = base & 0xFFFF;
	t->base_mid = (base >> 16) & 0xFF;
	t->flags1 = 0x89;	// present, type=0x9 (64비트 TSS, available)
	t->flags2 = 0x00;
	t->base_high = (base >> 24) & 0xFF;
	t->base_upper = (uint32_t)(base >> 32);
	t->reserved = 0;
}

void gdt_init(void) {
	memset(&tss, 0, sizeof(tss));
	tss.rsp0 = (uint64_t)(kernel_stack + sizeof(kernel_stack));
	tss.iomap_base = sizeof(struct tss_entry);	// I/O 비트맵 없음

	gdt_set_entry(0, 0, 0, 0, 0);				// null
	gdt_set_entry(1, 0, 0xFFFFF, 0x9A, 0xA0);		// kernel code, long mode
	gdt_set_entry(2, 0, 0xFFFFF, 0x92, 0xA0);		// kernel data
	gdt_set_entry(3, 0, 0xFFFFF, 0xF2, 0xA0);		// user data, DPL3 (gdt.h 참고: user code 앞이어야 함)
	gdt_set_entry(4, 0, 0xFFFFF, 0xFA, 0xA0);		// user code, DPL3
	gdt_set_tss(5, (uint64_t)&tss, sizeof(tss) - 1);	// 슬롯 5, 6을 차지

	gdtp.limit = sizeof(gdt) - 1;
	gdtp.base = (uint64_t)&gdt;

	gdt_flush((uint64_t)&gdtp);
	tss_flush(GDT_TSS_SEL);
}

void tss_set_kernel_stack(uint64_t rsp0) {
	tss.rsp0 = rsp0;
}
