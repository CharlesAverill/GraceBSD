#include "gdt.h"

#include <stddef.h>
#include <stdint.h>

#include "logging.h"
#include "tss.h"

struct gdt_entry {
    uint8_t* base;
    uint32_t limit;
    uint8_t access_byte;
    uint8_t flags;
};

// Size in bytes of an _encoded_ GDT entry
#define ENCODED_GDT_ENTRY_SIZE 8

// https://wiki.osdev.org/GDT_Tutorial#How_to_Set_Up_The_GDT
void encode_standard_gdt_entry(uint8_t* target, struct gdt_entry source) {
    // Check the limit to make sure that it can be encoded
    if (source.limit > 0xFFFFF) {
        fatal(RC_INIT_ERROR, "GDT cannot encode limits larger than 0xFFFFF");
    }

    // Encode the limit
    target[0] = source.limit & 0xFF;
    target[1] = (source.limit >> 8) & 0xFF;
    target[6] = (source.limit >> 16) & 0x0F;

    // Encode the base
    target[2] = (uint64_t)source.base & 0xFF;
    target[3] = ((uint64_t)source.base >> 8) & 0xFF;
    target[4] = ((uint64_t)source.base >> 16) & 0xFF;
    target[7] = ((uint64_t)source.base >> 24) & 0xFF;

    // Encode the access byte
    target[5] = source.access_byte;

    // Encode the flags
    target[6] |= (source.flags << 4);
}

// https://wiki.osdev.org/Global_Descriptor_Table#Long_Mode_System_Segment_Descriptor
void encode_large_gdt_entry(uint8_t* target, struct gdt_entry source) {
    // Check the limit to make sure that it can be encoded
    if (source.limit > 0xFFFFF) {
        fatal(RC_INIT_ERROR, "GDT cannot encode limits larger than 0xFFFFF");
    }

    // Encode the limit
    target[0] = source.limit & 0xFF;
    target[1] = (source.limit >> 8) & 0xFF;
    target[6] = (source.limit >> 16) & 0x0F;

    // Encode the base
    target[2]  = (uint64_t)source.base & 0xFF;
    target[3]  = ((uint64_t)source.base >> 8) & 0xFF;
    target[4]  = ((uint64_t)source.base >> 16) & 0xFF;
    target[7]  = ((uint64_t)source.base >> 24) & 0xFF;
    target[8]  = ((uint64_t)source.base >> 32) & 0xFF;
    target[9]  = ((uint64_t)source.base >> 40) & 0xFF;
    target[10] = ((uint64_t)source.base >> 48) & 0xFF;
    target[11] = ((uint64_t)source.base >> 56) & 0xFF;

    // Encode the access byte
    target[5] = source.access_byte;

    // Encode the flags
    target[6] |= (source.flags << 4);

    // Zero reserved bits
    target[12] = 0;
    target[13] = 0;
    target[14] = 0;
    target[15] = 0;
}

static struct tss tss;

// https://wiki.osdev.org/GDT_Tutorial#Flat_/_Long_Mode_Setup
#define N_STANDARD_GDT_ENTRIES 5
#define N_LARGE_GDT_ENTRIES    1
struct gdt_entry gdt_entries[N_STANDARD_GDT_ENTRIES + N_LARGE_GDT_ENTRIES] = {
    {.base = 0,               .limit = 0,               .access_byte = 0,    .flags = 0  }, // Null descriptor
    { .base = 0,              .limit = 0xFFFFF,         .access_byte = 0x9A, .flags = 0xA}, // Kernel code
    { .base = 0,              .limit = 0xFFFFF,         .access_byte = 0x92, .flags = 0xC}, // Kernel data
    { .base = 0,              .limit = 0xFFFFF,         .access_byte = 0xF2, .flags = 0xC}, // User data
    { .base = 0,              .limit = 0xFFFFF,         .access_byte = 0xFA, .flags = 0xA}, // User code
    { .base = (uint8_t*)&tss, .limit = sizeof(tss) - 1, .access_byte = 0x89, .flags = 0  }  // TSS
};

void setup_gdt(uint8_t* gdt) {
    for (size_t i = 0; i < N_STANDARD_GDT_ENTRIES; i++) {
        encode_standard_gdt_entry(gdt + ENCODED_GDT_ENTRY_SIZE * i, gdt_entries[i]);
    }

    for (size_t i = 0; i < N_LARGE_GDT_ENTRIES; i++) {
        encode_large_gdt_entry(gdt + ENCODED_GDT_ENTRY_SIZE * N_STANDARD_GDT_ENTRIES + 2 * ENCODED_GDT_ENTRY_SIZE * i,
                               gdt_entries[N_STANDARD_GDT_ENTRIES + i]);
    }
}

// https://wiki.osdev.org/Inline_Assembly/Examples#LIDT
static inline void lidt(void* base, uint16_t size) {
    struct {
        uint16_t length;
        void* base;
    } __attribute__((packed)) IDTR = { size, base };

    asm("lidt %0" : : "m"(IDTR));
}

static inline void lgdt(void* base, uint16_t size) {
    struct {
        uint16_t length;
        void* base;
    } __attribute__((packed)) IDTR = { size, base };

    asm("lgdt %0" : : "m"(IDTR) : "memory");
}

// https://wiki.osdev.org/GDT_Tutorial#Long_Mode_2
static inline void reload_CS(uint16_t sel) {
    __asm__ volatile("mov %0, %%ds" : : "r"(sel));
    __asm__ volatile("mov %0, %%es" : : "r"(sel));
    __asm__ volatile("mov %0, %%fs" : : "r"(sel));
    __asm__ volatile("mov %0, %%gs" : : "r"(sel));
    __asm__ volatile("mov %0, %%ss" : : "r"(sel));
}

static uint8_t gdt[ENCODED_GDT_ENTRY_SIZE * N_STANDARD_GDT_ENTRIES + 2 * ENCODED_GDT_ENTRY_SIZE * N_LARGE_GDT_ENTRIES]
    __attribute__((aligned(8)));

void load_gdt(void) {
    setup_gdt(gdt);
    lgdt(gdt, sizeof(gdt) - 1);

    // Far return into the next instruction with the new code selector
    __asm__ volatile("pushq %0\n"
                     "leaq 1f(%%rip), %%rax\n"
                     "pushq %%rax\n"
                     "lretq\n"
                     "1:"
                     :
                     : "i"(KERNEL_CODE_SELECTOR)
                     : "rax", "memory");

    reload_CS(KERNEL_DATA_SELECTOR);

    __asm__ volatile("ltr %0" : : "r"((uint16_t)GDT_TSS_SELECTOR));
}
