#include <danux/mm.h>
#include <stdint.h>

void *phys_to_virt(uint64_t phys) {
	return (void *) (phys + hhdm_offset);
}

uint64_t virt_to_phys(void *virt) {
	return (uint64_t) virt - hhdm_offset;
}

void *memset(void *base, int byte, uint64_t sz) {
	unsigned char *p = base;
	while (sz--) *p++ = byte;
	return base;
}
