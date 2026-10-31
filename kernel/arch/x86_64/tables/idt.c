#include "idt.h"

#include "gdt.h"
#include "logging.h"

__attribute__((aligned(0x10))) static idt_entry_t idt[256]; // Create an array of IDT entries; aligned for performance

static idtr_t idtr;

// Number of CPU exception vectors
#define N_EXCEPTIONS 32

// https://wiki.osdev.org/Exceptions
static const char* exception_names[N_EXCEPTIONS] = {
    "Division Error",
    "Debug",
    "Non-maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    "Reserved",
};

// https://wiki.osdev.org/Interrupt_Vector_Table#CPU_Interrupt_Layout
#define VECTOR_BREAKPOINT   3
#define VECTOR_DOUBLE_FAULT 8
#define VECTOR_PAGE_FAULT   14

static void log_registers(struct interrupt_frame* f) {
    error("  RIP=0x%016lx  CS=0x%lx  RFLAGS=0x%016lx", f->rip, f->cs, f->rflags);
    error("  RSP=0x%016lx  SS=0x%lx", f->rsp, f->ss);
    error("  RAX=0x%016lx  RBX=0x%016lx  RCX=0x%016lx", f->rax, f->rbx, f->rcx);
    error("  RDX=0x%016lx  RSI=0x%016lx  RDI=0x%016lx", f->rdx, f->rsi, f->rdi);
    error("  RBP=0x%016lx  R8 =0x%016lx  R9 =0x%016lx", f->rbp, f->r8, f->r9);
    error("  R10=0x%016lx  R11=0x%016lx  R12=0x%016lx", f->r10, f->r11, f->r12);
    error("  R13=0x%016lx  R14=0x%016lx  R15=0x%016lx", f->r13, f->r14, f->r15);
}

void exception_handler(struct interrupt_frame* frame) {
    const char* name = frame->vector < N_EXCEPTIONS ? exception_names[frame->vector] : "Unknown";

    // Resume from a breakpoint
    if (frame->vector == VECTOR_BREAKPOINT) {
        warn("Breakpoint at RIP=0x%lx", frame->rip);
        return;
    }

    error("%s (vector %lu, error code 0x%lx)", name, frame->vector, frame->error_code);
    // Get page address from cr2
    if (frame->vector == VECTOR_PAGE_FAULT) {
        // https://wiki.osdev.org/CPU_Registers_x86#CR2
        uint64_t cr2;
        __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
        error("  Faulting address (CR2) = 0x%016lx", cr2);
    }
    log_registers(frame);

    fatal(RC_ERROR, "Unhandled exception: %s", name);
}

void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags, uint8_t ist) {
    idt_entry_t* descriptor = &idt[vector];

    descriptor->isr_low    = (uint64_t)isr & 0xFFFF;
    descriptor->kernel_cs  = KERNEL_CODE_SELECTOR;
    descriptor->ist        = ist;
    descriptor->attributes = flags;
    descriptor->isr_mid    = ((uint64_t)isr >> 16) & 0xFFFF;
    descriptor->isr_high   = ((uint64_t)isr >> 32) & 0xFFFFFFFF;
    descriptor->reserved   = 0;
}

#define IDT_MAX_DESCRIPTORS 32

extern void* isr_stub_table[];

void idt_init(void) {
    idtr.base  = (uintptr_t)&idt[0];
    idtr.limit = (uint16_t)sizeof(idt_entry_t) * IDT_MAX_DESCRIPTORS - 1;

    for (uint8_t vector = 0; vector < N_EXCEPTIONS; vector++) {
        uint8_t ist = vector == VECTOR_DOUBLE_FAULT ? DOUBLE_FAULT_IST : 0;
        idt_set_descriptor(vector, isr_stub_table[vector], 0x8E, ist);
    }

    __asm__ volatile("lidt %0" : : "m"(idtr)); // load the new IDT
}
