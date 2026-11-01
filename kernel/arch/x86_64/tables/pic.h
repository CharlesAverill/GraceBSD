#ifndef GRACE_PIC_H
#define GRACE_PIC_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#define PIC1_OFFSET 0x20
#define PIC2_OFFSET 0x28

// https://wiki.osdev.org/X86_Interrupts#Standard_ISA_IRQs
// One bit per IRQ: bit n is IRQ n, so the master PIC is the low byte and the slave the high byte
// Master PIC (Port 0x21)
#define MASK_PIT      (1 << 0)
#define MASK_KEYBOARD (1 << 1)
#define MASK_CASCADE  (1 << 2)
#define MASK_COM2     (1 << 3)
#define MASK_COM1     (1 << 4)
#define MASK_LPT2     (1 << 5)
#define MASK_FLOPPY   (1 << 6)
#define MASK_LPT1     (1 << 7)

// Slave PIC (Port 0xA1)
#define MASK_CMOS          (1 << 8)
#define MASK_PERIPH1       (1 << 9)
#define MASK_PERIPH2       (1 << 10)
#define MASK_PERIPH3       (1 << 11)
#define MASK_PS2_MOUSE     (1 << 12)
#define MASK_FPU           (1 << 13)
#define MASK_PRIMARY_HDD   (1 << 14)
#define MASK_SECONDARY_HDD (1 << 15)

// The IRQ number of a MASK_* name, e.g. IRQ(KEYBOARD) is 1 and IRQ(CMOS) is 8
#define IRQ(mask) (__builtin_ctz(MASK_##mask))

void PIC_remap(int offset1, int offset2, uint16_t mask);

void PIC_sendEOI(uint8_t irq);
bool PIC_is_spurious(uint8_t irq);

#endif
