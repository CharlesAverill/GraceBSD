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

volatile int cakeml_result = 0;
volatile int cakeml_input  = 5;

void ffiinput_value(unsigned char* command, long command_length, unsigned char* args, long args_length) {
    (void)command;
    (void)command_length;

    if (args_length >= 1) {
        args[0] = cakeml_input;
    }
}

void ffireturn_value(unsigned char* command, long command_length, unsigned char* args, long args_length) {
    (void)command;
    (void)command_length;

    if (args_length >= 1) {
        cakeml_result = args[0];
    }
}

__attribute__((noreturn)) void cml_exit(int code) {
    char buf[12];
    int i = 0;

    serial_write("CakeML: cml_exit called, return value = ");
    code = cakeml_result;

    if (code < 0) {
        serial_write("-");
        code = -code;
    }

    do {
        buf[i++] = '0' + code % 10;
        code /= 10;
    } while (code > 0 && i < (int)sizeof(buf));

    while (i > 0) {
        char s[2] = { buf[--i], '\0' };
        serial_write(s);
    }

    serial_write("\n");

    for (;;) {
        __asm__ volatile("cli; hlt");
    }
}

#define MAX_INT_STR_LEN 21
void itoa(int x, char buf[MAX_INT_STR_LEN]) {
    int digits[MAX_INT_STR_LEN];
    int len;
    for (len = 0; x > 0 || len == 0; len++) {
        digits[len] = x % 10;
        x /= 10;
    }

    int i;
    for (i = 0; i < len + 1; i++) {
        buf[i] = '0' + digits[len - i];
    }
    buf[i] = '\0';
}

void cml_err(int arg) {
    char num[MAX_INT_STR_LEN];
    serial_write("CakeML returned with code ");
    serial_write(num);
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
