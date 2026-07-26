#ifndef DANUX_SERIAL_H
#define DANUX_SERIAL_H

#include <stdint.h>

extern void serial_init(void);           /* Initialize the UART. Call early during boot. */
extern void serial_putc(char c);         /* Send a single character. */
extern void serial_puts(const char *s);  /* Send a NUL-terminated string. */
extern void serial_puthex(uint64_t val); /* Print a value as 16-digit hexadecimal */
extern void serial_putdec(uint64_t val); /* Print an unsinged value in decimal */

#endif
