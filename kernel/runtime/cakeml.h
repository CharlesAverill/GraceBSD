#ifndef GRACE_CAKEML_H
#define GRACE_CAKEML_H

#include <stdint.h>

// 32 MB
#define CAKEML_HEAP_SIZE (32u * 1024u * 1024u)
// 8 MB
#define CAKEML_STACK_SIZE (8u * 1024u * 1024u)

// Start the CakeML kernel loop in a given memory region
void cakeml_run(uint8_t* mem_base, uint64_t mem_size);

// CakeML can't start up its own threads, resulting in some unpleasant interactions
// between features like event handling and blocking functions. So in the future,
// once multiple CakeML threads are needed, each thread will receive a "Job ID,"
// which is accessible through an FFI call. Each thread will start at the same
// cml_main entrypoint, will read job_id through FFI, then dispatch to its roles.
extern uint8_t job_id;

#endif
