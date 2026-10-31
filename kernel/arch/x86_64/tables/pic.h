#ifndef GRACE_PIC_H
#define GRACE_PIC_H

#include <stdbool.h>
#include <stdint.h>

#define PIC1_OFFSET 0x20
#define PIC2_OFFSET 0x28

void PIC_remap(int offset1, int offset2);
void PIC_sendEOI(uint8_t irq);
bool PIC_is_spurious(uint8_t irq);

#endif
