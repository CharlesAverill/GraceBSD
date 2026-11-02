#ifndef GRACE_HW_H
#define GRACE_HW_H

#include <limine.h>
#include <stddef.h>
#include <stdint.h>

__attribute__((noreturn)) void hcf(void);

// Memory map entries
extern LIMINE_PTR(struct limine_memmap_entry**) mmap_entries;
extern size_t num_mmap_entries;

// HHDM offset
extern uint64_t hhdm_off;

// Where Limine loaded the kernel image
extern uint64_t kernel_phys_base;
extern uint64_t kernel_virt_base;

// Segment boundaries from linker.ld, as virtual addresses. Each segment runs
// up to the start of the next; the data segment includes .bss.
extern char kernel_start[], kernel_text_start[], kernel_rodata_start[], kernel_data_start[], kernel_end[];

// CakeML stack and heap
extern uint8_t* cml_heap;
extern uint8_t* cml_stack;
extern uint8_t* cml_stackend;

// Framebuffers
extern struct limine_framebuffer** framebuffers;
extern size_t num_framebuffers;
#define FB_MAIN (framebuffers[0])

#endif
