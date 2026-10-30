#ifndef GRACE_SERIAL_H
#define GRACE_SERIAL_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

int serial_init(void);
void serial_putc(char c);
char serial_getc(void);
void serial_write(char* s);
void serial_writen(char* s, size_t len);
size_t serial_readn(char* s, size_t len);
void serial_write_int(int64_t x);
void serial_write_hex(uint64_t x);
int64_t serial_read_int(void);

// printf to COM1 using nanoprintf
__attribute__((format(printf, 1, 0))) void serial_vprintf(const char* fmt, va_list args);
__attribute__((format(printf, 1, 2))) void serial_printf(const char* fmt, ...);

#define COM1 0x3F8

#endif
