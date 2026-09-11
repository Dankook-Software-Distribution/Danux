#ifndef DANUX_TIMER_H
#define DANUX_TIMER_H

#include <danux/idt.h>
#include <stdint.h>

extern volatile uint64_t timer_ticks;

extern void timer_init(uint32_t freq_hz);
extern void timer_handler(registers_t *regs);

#endif
