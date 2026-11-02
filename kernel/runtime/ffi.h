#ifndef GRACE_FFI_H
#define GRACE_FFI_H

#include <stdint.h>

#define FFI(f) void ffi##f(unsigned char* command, long command_length, uint8_t* args, long args_length)

#define SIZE_OF_INT64 8

uint64_t uint64_of_bytes(unsigned char bytes[]);
void bytes_of_uint64(uint64_t n, unsigned char bytes[]);

#endif
