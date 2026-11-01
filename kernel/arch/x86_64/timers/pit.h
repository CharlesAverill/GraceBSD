#ifndef GRACE_PIT_H
#define GRACE_PIT_H

#include <stdint.h>

#define DESIRED_PIT_FREQ 100

// PIT input clock speed
#define PIT_RAW_HZ 1193182

// The programmed divisor of PIT_RAW_HZ
// 1 tick = pit_reload / PIT_RAW_HZ seconds
extern uint32_t pit_reload;

void set_pit_freq(uint16_t hz);

#endif
