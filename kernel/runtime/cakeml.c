#include "cakeml.h"

#include <stdint.h>

#include "hw.h"
#include "logging.h"

extern int cml_main(void);

void cml_clear(void) { }

__attribute__((noreturn)) void cml_exit(int code) {
    fatal(RC_ERROR, "CakeML exited with code %d", code);
}

void cml_err(int arg) {
    fatal(RC_ERROR, "CakeML runtime error %d", arg);
}

void cakeml_run(uint8_t* mem_base, uint64_t mem_size) {
    debug("CakeML: setting up memory");

    cml_heap     = mem_base;
    cml_stack    = cml_heap + CAKEML_HEAP_SIZE;
    cml_stackend = cml_stack + CAKEML_STACK_SIZE;

    debug("CakeML: calling cml_main");
    int rc = cml_main();

    log(rc == 0 ? LOG_INFO : LOG_ERROR, "CakeML: cml_main returned with code %d", rc);
}
