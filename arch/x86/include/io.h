#ifndef IO_H
#define IO_H

#include <stdint.h>

#define BUILDIO(bwl, bw, type)                                                 \
static inline void __out##bwl(type value, uint16_t port) {                     \
	asm volatile("out" #bwl " %" #bw "0, %w1" : : "a"(value), "Nd"(port)); \
}                                                                              \
                                                                               \
static inline type __in##bwl(uint16_t port) {                                  \
	type value;                                                            \
	asm volatile("in" #bwl " %w1, %" #bw "0" : "=a"(value) : "Nd"(port));  \
	return value;                                                          \
}

BUILDIO(b, b, uint8_t)
BUILDIO(w, w, uint16_t)
BUILDIO(l,  , uint32_t)
#undef BUILDIO

#define inb  __inb
#define inw  __inw
#define inl  __inl
#define outb __outb
#define outw __outw
#define outl __outl

#endif
