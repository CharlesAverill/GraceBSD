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

// CakeML stack and heap
extern uint8_t* cml_heap;
extern uint8_t* cml_stack;
extern uint8_t* cml_stackend;

// Framebuffers
extern struct limine_framebuffer** framebuffers;
extern size_t num_framebuffers;
#define FB_MAIN (framebuffers[0])

#endif
