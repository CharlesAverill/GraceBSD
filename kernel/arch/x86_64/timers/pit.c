#include "pit.h"

#include "io.h"

// The PIT's input clock in Hz
#define PIT_RAW_HZ 1193182

// Mode 2 needs a divisor of at least 2
#define PIT_MIN_RELOAD 2
#define PIT_MAX_RELOAD 65536

uint16_t pit_freq = 0;

void set_pit_freq(uint16_t hz) {
    // Round to the nearest divisor and clamp
    uint32_t reload = hz == 0 ? PIT_MAX_RELOAD : (PIT_RAW_HZ + hz / 2) / hz;
    if (reload < PIT_MIN_RELOAD) {
        reload = PIT_MIN_RELOAD;
    }
    if (reload > PIT_MAX_RELOAD) {
        reload = PIT_MAX_RELOAD;
    }
    pit_freq = PIT_RAW_HZ / reload;

    // Command byte
    outb(0x43, 0x34);

    // Set low byte
    outb(0x40, reload & 0xFF);          // Low byte
    outb(0x40, (reload & 0xFF00) >> 8); // High byte
    return;
}
