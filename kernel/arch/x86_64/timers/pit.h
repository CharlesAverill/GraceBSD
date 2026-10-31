#ifndef GRACE_PIT_H
#define GRACE_PIT_H

#include <stdint.h>

#define DESIRED_PIT_FREQ 100

extern uint16_t pit_freq;

void set_pit_freq(uint16_t hz);

#endif
