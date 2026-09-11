#include <danux/string.h>
#include <stddef.h>

void *memcpy(void *dst, const void *src, size_t n) {
	unsigned char *d = dst;
	const unsigned char *s = src;
	while (n--) *d++ = *s++;
	return dst;
}

size_t strlen(const char *s) {
	size_t n = 0;
	while (s[n]) n++;
	return n;
}

int strcmp(const char *a, const char *b) {
	while (*a && (*a == *b)) { a++; b++; }
	return (unsigned char)*a - (unsigned char)*b;
}
