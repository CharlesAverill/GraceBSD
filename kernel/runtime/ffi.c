#include "memory.h"
#include "serial.h"
#include <stdint.h>

#define FFI(f) void ffi##f(unsigned char* command, long command_length, unsigned char* args, long args_length)

FFI(serial_write) {
    serial_writen((char*)command, command_length);
}

#define SIZE_OF_INT64 8
uint64_t uint64_of_bytes(unsigned char bytes[]) {
    uint64_t bits = 0;
    for (int i = SIZE_OF_INT64 - 1; i >= 0; i--) {
        bits = (bits << 8) | bytes[i];
    }
    return bits;
}

void bytes_of_uint64(uint64_t n, unsigned char bytes[]) {
    for (int i = 0; i < SIZE_OF_INT64; i++) {
        bytes[i] = n & 0xff;
        n >>= 8;
    }
}

// Read a fixed number of bytes from COM1, stop reading upon a newline or CRE
FFI(serial_readn) {
    if (args_length < SIZE_OF_INT64) {
        serial_write("serial_readn: expected at least 8 bytes\n");
        return;
    }

    uint64_t len = serial_readn((char*)(args + SIZE_OF_INT64), args_length - SIZE_OF_INT64);
    bytes_of_uint64(len, args);
}

// args holds a signed 64-bit integer as 8 bytes of little-endian two's complement
FFI(serial_write_int) {
    if (args_length != SIZE_OF_INT64) {
        serial_write("serial_write_int: expected 8 bytes\n");
        return;
    }

    serial_write_int((int64_t)uint64_of_bytes((unsigned char*)args));
}

// args holds an unsigned 64-bit integer as 8 bytes of little-endian
FFI(serial_write_hex) {
    if (args_length != SIZE_OF_INT64) {
        serial_write("serial_write_hex: expected 8 bytes\n");
        return;
    }

    serial_write_hex(uint64_of_bytes((unsigned char*)args));
}

// Get HHDM offset
FFI(get_hhdm) {
    if (args_length != SIZE_OF_INT64) {
        serial_write("get_hhdm: expected 8 bytes\n");
        return;
    }

    bytes_of_uint64(hhdm_off, args);
}

// args holds the address as 8 bytes, then the space to read into.
FFI(mem_read) {
    if (args_length < SIZE_OF_INT64) {
        serial_write("mem_read: expected at least 8 bytes\n");
        return;
    }

    char* addr = (char*)uint64_of_bytes(args);
    for (long i = 0; i < args_length - SIZE_OF_INT64; i++) {
        args[SIZE_OF_INT64 + i] = addr[i];
    }
}

// Read 32 bits from memory
FFI(mem_read32) {
    if (args_length < SIZE_OF_INT64) {
        serial_write("mem_read32: expected 8 bytes\n");
        return;
    }

    volatile uint32_t* addr = (uint32_t*)uint64_of_bytes(args);
    bytes_of_uint64((uint64_t)(*addr), args);
}

// Read 64 bits from memory
FFI(mem_read64) {
    if (args_length < SIZE_OF_INT64) {
        serial_write("mem_read64: expected 8 bytes\n");
        return;
    }

    volatile uint64_t* addr = (uint64_t*)uint64_of_bytes(args);
    bytes_of_uint64(*addr, args);
}
