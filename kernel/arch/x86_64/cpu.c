#include "cpu.h"

#include "ffi.h"
#include "logging.h"

// https://wiki.osdev.org/CPU_Registers_x86-64#CR3
uint64_t read_cr3(void) {
    uint64_t value;
    __asm__ volatile("mov %%cr3, %0" : "=r"(value));
    return value;
}

// Switch to the page tables rooted at [value]
void load_cr3(uint64_t value) {
    __asm__ volatile("mov %0, %%cr3" : : "r"(value) : "memory");
}

// Drop the TLB entry for the page containing [addr]
void invlpg(uint64_t addr) {
    __asm__ volatile("invlpg (%0)" : : "r"(addr) : "memory");
}

FFI(read_cr3) {
    if (args_length != SIZE_OF_INT64) {
        error("FFI read_cr3: expected 8 bytes");
        return;
    }

    bytes_of_uint64(read_cr3(), args);
}

FFI(load_cr3) {
    if (args_length != SIZE_OF_INT64) {
        error("FFI load_cr3: expected 8 bytes");
        return;
    }

    load_cr3(uint64_of_bytes(args));
}

FFI(invlpg) {
    if (args_length != SIZE_OF_INT64) {
        error("FFI invlpg: expected 8 bytes");
        return;
    }

    invlpg(uint64_of_bytes(args));
}
