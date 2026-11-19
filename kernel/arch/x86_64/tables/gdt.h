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

// Stack for double faults
#define DOUBLE_FAULT_STACK_SIZE (16 * 1024)
// Stack for NMIs
#define NMI_STACK_SIZE (16 * 1024)
// Stack for Machine Checks
#define MACHINE_CHECK_STACK_SIZE (16 * 1024)
// Stack for Ring 3 interrupts
#define R3_INT_STACK_SIZE (16 * 1024)

void load_gdt(void);

#endif
