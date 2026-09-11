#ifndef DANUX_STRING_H
#define DANUX_STRING_H

#include <stddef.h>

// memset은 init/mm.c에 이미 있음 (danux/mm.h 참고).
extern void *memcpy(void *dst, const void *src, size_t n);
extern size_t strlen(const char *s);
extern int strcmp(const char *a, const char *b);

#endif
