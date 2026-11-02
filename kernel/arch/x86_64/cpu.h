#ifndef GRACE_CPU_H
#define GRACE_CPU_H

#include <stdint.h>

uint64_t read_cr3(void);
void load_cr3(uint64_t value);
void invlpg(uint64_t addr);

#endif
