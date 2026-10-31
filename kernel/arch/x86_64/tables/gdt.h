#ifndef GRACE_GDT_H
#define GRACE_GDT_H

#define KERNEL_CODE_SELECTOR 0x08
#define KERNEL_DATA_SELECTOR 0x10
#define GDT_TSS_SELECTOR     0x28

void load_gdt(void);

#endif
