#include <danux/serial.h>
#include <io.h>
#include <stdint.h>

/*
 * Detailed explanations regarding the constants and serial manipulation logic can be found here:
 * https://wiki.osdev.org/Serial_Ports#Programming_the_Serial_Communications_Port
 */

#define COM1		0x3F8
#define UART_TX		0	/* Out: Transmit buffer */
#define UART_DLL	0	/* Out: Divisor Latch Low */
#define UART_IER	1	/* Out: Interrupt Enable Register */
#define UART_DLM	1	/* Out: Divisor Latch High */
#define UART_FCR	2	/* Out: FIFO Control Register */
#define UART_LCR	3	/* Out: Line Control Register */
#define UART_MCR	4	/* Out: Modem Control Register */
#define UART_LSR	5	/* In:	Line Status Register */

#define UART_LCR_DLAB		0x80 /* Divisor latch access bit */
#define UART_LSR_THRE		0x20 /* Transmit-hold-register empty */
#define UART_DIVISOR 		1 	 /* 115200 baud: divisor = 115200 / baud = 1. */

static const char hex_digits[] = "0123456789abcdef";

void serial_init(void) {
	outb(0x00, COM1 + UART_IER);            /* disable interrupts (polling) */

	outb(UART_LCR_DLAB, COM1 + UART_LCR);   /* enable divisor latch */
	outb(UART_DIVISOR & 0xFF, COM1 + UART_DLL);
	outb((UART_DIVISOR >> 8) & 0xFF, COM1 + UART_DLM);

	outb(0x03, COM1 + UART_LCR);    	    /* 8n1, clears DLAB */
	outb(0xC7, COM1 + UART_FCR);            /* enable FIFO buffering. */
	outb(0x03, COM1 + UART_MCR);            /* DTR + RTS */
}

void serial_putc(char c) {
	/* Translate LF into CR/LF for terminals. */
	if (c == '\n')
		serial_putc('\r');

	while ((inb(COM1 + UART_LSR) & UART_LSR_THRE) == 0)
		; 	/* wait until THR is empty */
	outb((uint8_t)c, COM1 + UART_TX);
}

void serial_puts(const char *s) {
	while (*s)
		serial_putc(*s++);
}

void serial_puthex(uint64_t val) {
	serial_puts("0x");
	for (int i = 60; i >= 0; i -= 4)
		serial_putc(hex_digits[(val >> i) & 0xF]);
}

void serial_putdec(uint64_t val) {
	char buf[20];
	int i = 0;

	if (val == 0) {
		serial_putc('0');
		return;
	}

	while (val > 0) {
		buf[i++] = '0' + (val % 10);
		val /= 10;
	}

	while (i > 0) 				/* LSB-first */
		serial_putc(buf[--i]);
}
