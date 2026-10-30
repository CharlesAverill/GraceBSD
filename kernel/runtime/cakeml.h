#ifndef GRACE_CAKEML_H
#define GRACE_CAKEML_H

#include <stdint.h>

// 4 MB
#define CAKEML_HEAP_SIZE (4u * 1024u * 1024u)
// 1 MB
#define CAKEML_STACK_SIZE (1u * 1024u * 1024u)

// Start the CakeML kernel loop in a given memory region
void cakeml_run(char* mem_base, uint64_t mem_size);

#endif
