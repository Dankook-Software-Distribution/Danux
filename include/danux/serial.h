#ifndef DANUX_SERIAL_H
#define DANUX_SERIAL_H

extern void serial_init(void);          /* Initialize the UART. Call early during boot. */
extern void serial_putc(char c);        /* Send a single character. */
extern void serial_puts(const char *s); /* Send a NUL-terminated string. */

#endif
