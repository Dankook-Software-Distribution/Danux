#ifndef DANUX_PIC_H
#define DANUX_PIC_H

#include <stdint.h>

extern void pic_remap(int offset1, int offset2);
extern void pic_send_eoi(uint8_t irq);
extern void pic_set_mask(uint8_t irq);
extern void pic_clear_mask(uint8_t irq);

#endif
