#ifndef GRACE_SERIAL_H
#define GRACE_SERIAL_H

#include <stddef.h>
#include <stdint.h>

int serial_init(void);
void serial_putc(char c);
char serial_getc(void);
void serial_write(char* s);
void serial_writen(char* s, size_t len);
void serial_readn(char* s, size_t len);
void serial_write_int(int64_t x);
int64_t serial_read_int(void);

#define COM1 0x3F8

#endif
