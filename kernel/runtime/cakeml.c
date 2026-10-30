#include "cakeml.h"
#include "serial.h"
#include <stdint.h>

extern char* cml_heap;
extern char* cml_stack;
extern char* cml_stackend;

extern int cml_main(void);

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

void cakeml_run(char* mem_base, uint64_t mem_size) {
    serial_write("CakeML: setting up memory\n");

    cml_heap     = mem_base;
    cml_stack    = cml_heap + CAKEML_HEAP_SIZE;
    cml_stackend = cml_stack + CAKEML_STACK_SIZE;

    serial_write("CakeML: calling cml_main\n");
    int rc = cml_main();

    serial_write("CakeML: cml_main returned with code = ");
    serial_write_int(rc);
    serial_write("\n");
}
