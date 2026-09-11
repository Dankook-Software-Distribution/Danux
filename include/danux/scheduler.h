#ifndef DANUX_SCHEDULER_H
#define DANUX_SCHEDULER_H

#include <danux/idt.h>
#include <danux/process.h>

extern process_t *current_process;

extern void scheduler_init(void);
extern void scheduler_add(process_t *proc);

// IRQ0(타이머) 핸들러로 등록됨: round-robin으로 다음 READY 프로세스를 골라
// 컨텍스트 스위치한다.
extern void scheduler_tick(registers_t *regs);

// scheduler_switch.S에 구현됨
extern void context_switch(uint64_t *old_rsp_store, uint64_t new_rsp);

#endif
