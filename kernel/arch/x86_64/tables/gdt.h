#ifndef GRACE_GDT_H
#define GRACE_GDT_H

#define KERNEL_CODE_SELECTOR 0x08
#define KERNEL_DATA_SELECTOR 0x10
#define GDT_TSS_SELECTOR     0x28

// The IST entry for double faults
#define DOUBLE_FAULT_IST 1

void load_gdt(void);

#endif
