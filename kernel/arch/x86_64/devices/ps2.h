#ifndef GRACE_PS2_H
#define GRACE_PS2_H

#include <stdbool.h>

// Initialise the 8042 controller and the keyboard on its first port, return true
// if initialized, or false if no keyboard was set up
bool ps2_init(void);

#endif
