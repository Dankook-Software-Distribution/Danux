#include <danux/mm.h>
#include <stdint.h>

uint64_t *phys_to_virt(uint64_t phys) {
	return (uint64_t *) (phys + hhdm_offset);
}

void memset(void *base, int byte, uint64_t sz) {
	unsigned char *p = base;
	while (sz--) *p++ = byte;
}
