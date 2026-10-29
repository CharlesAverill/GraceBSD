#ifndef GRACE_SERIAL_H
#define GRACE_SERIAL_H

int serial_init(void);
void serial_putc(char c);
void serial_write(const char* s);

#define COM1 0x3F8

#endif
