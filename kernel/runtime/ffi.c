#include <stdint.h>

#include "serial.h"

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

// Read a fixed number of bytes from COM1, stop reading upon a newline or CRE
FFI(serial_readn) {
    if (args_length < SIZE_OF_INT64) {
        serial_write("serial_readn: expected length to read\n");
        return;
    }

    uint64_t len = uint64_of_bytes((unsigned char*)args);

    serial_readn((char*)(args + SIZE_OF_INT64), len);
}

// args holds a signed 64-bit integer as 8 bytes of little-endian two's complement
FFI(serial_write_int) {
    if (args_length != SIZE_OF_INT64) {
        serial_write("serial_write_int: expected 8 bytes\n");
        return;
    }

    serial_write_int((int64_t)uint64_of_bytes((unsigned char*)args));
}
