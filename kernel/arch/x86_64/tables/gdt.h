#ifndef GRACE_GDT_H
#define GRACE_GDT_H

#define KERNEL_CODE_SELECTOR 0x08
#define KERNEL_DATA_SELECTOR 0x10
#define GDT_TSS_SELECTOR     0x28

// IST entry positions
#define DOUBLE_FAULT_IST  1
#define NMI_IST           2
#define DEBUG_IST         3
#define MACHINE_CHECK_IST 4

void load_gdt(void);

#endif
