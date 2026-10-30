#include "serial.h"
#include <stdint.h>

#define CAKEML_HEAP_SIZE  (1u * 1024u * 1024u)
#define CAKEML_STACK_SIZE (1u * 1024u * 1024u)

extern char* cml_heap;
extern char* cml_stack;
extern char* cml_stackend;

extern void cml_main(void);

static uint8_t cakeml_memory[CAKEML_HEAP_SIZE + CAKEML_STACK_SIZE] __attribute__((aligned(16)));

void cml_clear(void) { }

__attribute__((noreturn)) void cml_exit(int code) {
    serial_write("CakeML: cml_exit\n");

    for (;;) {
        __asm__ volatile("cli; hlt");
    }
}

void cml_err(int arg) {
    serial_write("CakeML returned with code ");
    serial_write_int(arg);
    serial_write("\n");

    cml_exit(arg);
}

void cakeml_run(void) {
    serial_write("CakeML: setting up memory\n");

    cml_heap     = (char*)cakeml_memory;
    cml_stack    = cml_heap + CAKEML_HEAP_SIZE;
    cml_stackend = cml_stack + CAKEML_STACK_SIZE;

    serial_write("CakeML: calling cml_main\n");
    cml_main();

    serial_write("CakeML: cml_main returned\n");
}
