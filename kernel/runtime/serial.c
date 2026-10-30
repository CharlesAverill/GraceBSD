#include "serial.h"

#include "nanoprintf.h"

#include <stdbool.h>
#include <stdint.h>

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

int serial_init() {
    outb(COM1 + 1, 0x00); // Disable all interrupts
    outb(COM1 + 3, 0x80); // Enable DLAB (set baud rate divisor)
    outb(COM1 + 0, 0x03); // Set divisor to 3 (lo byte) 38400 baud
    outb(COM1 + 1, 0x00); //                  (hi byte)
    outb(COM1 + 3, 0x03); // 8 bits, no parity, one stop bit
    outb(COM1 + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
    outb(COM1 + 4, 0x0B); // IRQs enabled, RTS/DSR set
    outb(COM1 + 4, 0x1E); // Set in loopback mode, test the serial chip
    outb(COM1 + 0, 0xAE); // Test serial chip (send byte 0xAE and check if serial returns same byte)

    // Check if serial is faulty (i.e: not same byte as sent)
    if (inb(COM1 + 0) != 0xAE) {
        return 1;
    }

    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    outb(COM1 + 4, 0x0F);
    return 0;
}

static bool serial_received(void) {
    return inb(COM1 + 5) & 1;
}

static bool is_transmit_empty(void) {
    return (inb(COM1 + 5) & 0x20);
}

void serial_putc(char c) {
    while (!is_transmit_empty())
        ;

    outb(COM1, c);
}

char serial_getc(void) {
    while (!serial_received())
        ;

    char c = inb(COM1);
    if (c == '\r')
        serial_putc('\n');
    serial_putc(c);
    return c;
}

void serial_write(char* s) {
    while (*s) {
        if (*s == '\n') {
            serial_putc('\r');
        }
        serial_putc(*s++);
    }
}

void serial_writen(char* s, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (*s == '\n') {
            serial_putc('\r');
        }
        serial_putc(*s++);
    }
}

size_t serial_readn(char* s, size_t len) {
    size_t i;
    for (i = 0; i < len; i++) {
        s[i] = serial_getc();
        if (s[i] == '\n' || s[i] == '\r')
            break;
    }
    return i;
}

// Sign, 19 digits for INT64_MIN, and the terminator.
#define MAX_INT_STR_LEN 21

void serial_write_int(int64_t x) {
    serial_printf("%lld", (long long)x);
}

void serial_write_hex(uint64_t x) {
    serial_printf("0x%llx", (unsigned long long)x);
}

int64_t atol(char* s) {
    long n  = 0;
    int neg = 0;

    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r' || *s == '\v' || *s == '\f') {
        s++;
    }

    if (*s == '-') {
        neg = 1;
        s++;
    } else if (*s == '+') {
        s++;
    }

    while (*s >= '0' && *s <= '9') {
        n = n * 10 + (*s - '0');
        s++;
    }

    return neg ? -n : n;
}

int64_t serial_read_int(void) {
    char buf[MAX_INT_STR_LEN];
    serial_readn(buf, MAX_INT_STR_LEN);
    return atol(buf);
}

static void serial_npf_putc(int c, void* ctx) {
    (void)ctx;
    if (c == '\n') {
        serial_putc('\r');
    }
    serial_putc((char)c);
}

void serial_vprintf(const char* fmt, va_list args) {
    npf_vpprintf(serial_npf_putc, NULL, fmt, args);
}

void serial_printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    serial_vprintf(fmt, args);
    va_end(args);
}
