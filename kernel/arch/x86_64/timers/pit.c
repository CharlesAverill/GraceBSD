#include "pit.h"

#include "io.h"

// Mode 2 needs a divisor of at least 2
#define PIT_MIN_RELOAD 2
#define PIT_MAX_RELOAD 65536

uint32_t pit_reload = 0;

void set_pit_freq(uint16_t hz) {
    // Round to the nearest divisor and clamp
    pit_reload = hz == 0 ? PIT_MAX_RELOAD : (PIT_RAW_HZ + hz / 2) / hz;
    if (pit_reload < PIT_MIN_RELOAD) {
        pit_reload = PIT_MIN_RELOAD;
    }
    if (pit_reload > PIT_MAX_RELOAD) {
        pit_reload = PIT_MAX_RELOAD;
    }
    pit_reload = pit_reload;

    // Command byte
    outb(0x43, 0x34);

    // Set low byte
    outb(0x40, pit_reload & 0xFF);          // Low byte
    outb(0x40, (pit_reload & 0xFF00) >> 8); // High byte
    return;
}
