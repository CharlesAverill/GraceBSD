#include <stdint.h>

#include "fonts.h"
#include "framebuffer.h"
#include "hw.h"
#include "logging.h"
#include "serial.h"

#define FFI(f) void ffi##f(unsigned char* command, long command_length, uint8_t* args, long args_length)

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
        error("FFI serial_readn: expected at least 8 bytes");
        return;
    }

    uint64_t len = serial_readn((char*)(args + SIZE_OF_INT64), args_length - SIZE_OF_INT64);
    bytes_of_uint64(len, args);
}

FFI(serial_read_char) {
    if (args_length != 1) {
        error("FFI serial_read_char: expected 1 byte");
        return;
    }

    args[0] = (unsigned char)serial_getc_raw();
}

// args holds a signed 64-bit integer as 8 bytes of little-endian two's complement
FFI(serial_write_int) {
    if (args_length != SIZE_OF_INT64) {
        error("FFI serial_write_int: expected 8 bytes");
        return;
    }

    serial_write_int((int64_t)uint64_of_bytes(args));
}

// args holds an unsigned 64-bit integer as 8 bytes of little-endian
FFI(serial_write_hex) {
    if (args_length != SIZE_OF_INT64) {
        error("FFI serial_write_hex: expected 8 bytes");
        return;
    }

    serial_write_hex(uint64_of_bytes(args));
}

// Get HHDM offset
FFI(get_hhdm) {
    if (args_length != SIZE_OF_INT64) {
        error("FFI get_hhdm: expected 8 bytes");
        return;
    }

    bytes_of_uint64(hhdm_off, args);
}

// args holds the address as 8 bytes, then the space to read into
FFI(mem_read) {
    if (args_length < SIZE_OF_INT64) {
        error("FFI mem_read: expected at least 8 bytes");
        return;
    }

    uint8_t* addr = (uint8_t*)uint64_of_bytes(args);
    for (long i = 0; i < args_length - SIZE_OF_INT64; i++) {
        args[SIZE_OF_INT64 + i] = addr[i];
    }
}

// Read 32 bits from memory
FFI(mem_read32) {
    if (args_length != SIZE_OF_INT64) {
        error("FFI mem_read32: expected 8 bytes");
        return;
    }

    volatile uint32_t* addr = (uint32_t*)uint64_of_bytes(args);
    bytes_of_uint64((uint64_t)(*addr), args);
}

// Read 64 bits from memory
FFI(mem_read64) {
    if (args_length != SIZE_OF_INT64) {
        error("FFI mem_read64: expected 8 bytes");
        return;
    }

    volatile uint64_t* addr = (uint64_t*)uint64_of_bytes(args);
    bytes_of_uint64(*addr, args);
}

// args holds the address as 8 bytes, then the bytes to write
FFI(mem_write) {
    if (args_length < SIZE_OF_INT64) {
        error("FFI mem_write: expected at least 8 bytes");
        return;
    }

    uint8_t* addr = (uint8_t*)uint64_of_bytes(args);
    for (long i = 0; i < args_length - SIZE_OF_INT64; i++) {
        addr[i] = args[SIZE_OF_INT64 + i];
    }
}

// Write 32 bits into memory
FFI(mem_write32) {
    if (args_length != 2 * SIZE_OF_INT64) {
        error("FFI mem_write32: expected 16 bytes");
        return;
    }

    volatile uint32_t* addr = (uint32_t*)uint64_of_bytes(args);
    *addr                   = (uint32_t)uint64_of_bytes(args + SIZE_OF_INT64);
}

// Write 64 bits into memory
FFI(mem_write64) {
    if (args_length != 2 * SIZE_OF_INT64) {
        error("FFI mem_write64: expected 16 bytes");
        return;
    }

    volatile uint64_t* addr = (uint64_t*)uint64_of_bytes(args);
    *addr                   = (uint64_t)uint64_of_bytes(args + SIZE_OF_INT64);
}

// Halt and Catch Fire
FFI(hcf) {
    hcf();
}

FFI(log) {
    if (args_length != SIZE_OF_INT64) {
        error("FFI log: expected 8 bytes");
        return;
    }

    log((LogLevel)uint64_of_bytes(args), "%.*s", (int)command_length, (char*)command);
}

FFI(fatal) {
    if (args_length != SIZE_OF_INT64) {
        error("FFI fatal: expected 8 bytes");
        return;
    }

    fatal((ReturnCode)uint64_of_bytes(args), "%.*s", (int)command_length, (char*)command);
}

FFI(n_mmap_entries) {
    if (args_length != SIZE_OF_INT64) {
        error("FFI n_mmap_entries: expected 8 bytes");
        return;
    }

    bytes_of_uint64(num_mmap_entries, args);
}

FFI(load_mmap_entry) {
    if (args_length != 3 * SIZE_OF_INT64) {
        error("FFI load_mmap_entry: expected 24 bytes");
        return;
    }

    size_t i = uint64_of_bytes(args);
    if (i >= num_mmap_entries) {
        error("FFI load_mmap_entry: index %lu out of range", i);
        return;
    }

    struct limine_memmap_entry* mmap_entry = mmap_entries[i];
    bytes_of_uint64(mmap_entry->base, args);
    bytes_of_uint64(mmap_entry->length, args + SIZE_OF_INT64);
    bytes_of_uint64(mmap_entry->type, args + 2 * SIZE_OF_INT64);
}

FFI(n_framebuffers) {
    if (args_length != SIZE_OF_INT64) {
        error("FFI n_framebuffers: expected 8 bytes");
        return;
    }

    bytes_of_uint64(num_framebuffers, args);
}

// The number of 8-byte fields returned by load_framebuffer
#define FRAMEBUFFER_FIELDS 12

// args holds the index in its first 8 bytes, and receives the framebuffer's
// address, width, height, pitch, bpp and memory model, then the size and shift
// of its red, green and blue masks
FFI(load_framebuffer) {
    if (args_length != FRAMEBUFFER_FIELDS * SIZE_OF_INT64) {
        error("FFI load_framebuffer: expected %d bytes", FRAMEBUFFER_FIELDS * SIZE_OF_INT64);
        return;
    }

    size_t i = uint64_of_bytes(args);
    if (i >= num_framebuffers) {
        error("FFI load_framebuffer: index %lu out of range", i);
        return;
    }

    struct limine_framebuffer* fb       = framebuffers[i];
    uint64_t fields[FRAMEBUFFER_FIELDS] = {
        (uint64_t)fb->address, fb->width,           fb->height,         fb->pitch,           fb->bpp,
        fb->memory_model,      fb->red_mask_size,   fb->red_mask_shift, fb->green_mask_size, fb->green_mask_shift,
        fb->blue_mask_size,    fb->blue_mask_shift,
    };
    for (int k = 0; k < FRAMEBUFFER_FIELDS; k++) {
        bytes_of_uint64(fields[k], args + k * SIZE_OF_INT64);
    }
}

FFI(cml_heap) {
    if (args_length != SIZE_OF_INT64) {
        error("FFI cml_heap: expected 8 bytes");
        return;
    }

    bytes_of_uint64((uint64_t)cml_heap, args);
}

FFI(cml_stack) {
    if (args_length != SIZE_OF_INT64) {
        error("FFI cml_stack: expected 8 bytes");
        return;
    }

    bytes_of_uint64((uint64_t)cml_stack, args);
}

FFI(cml_stackend) {
    if (args_length != SIZE_OF_INT64) {
        error("FFI cml_stackend: expected 8 bytes");
        return;
    }

    bytes_of_uint64((uint64_t)cml_stackend, args);
}

FFI(width_of_font) {
    if (args_length != SIZE_OF_INT64) {
        error("FFI width_of_font: expected 8 bytes");
        return;
    }

    bytes_of_uint64(width_of_Font((Font)uint64_of_bytes(args)), args);
}

FFI(height_of_font) {
    if (args_length != SIZE_OF_INT64) {
        error("FFI height_of_font: expected 8 bytes");
        return;
    }

    bytes_of_uint64(height_of_Font((Font)uint64_of_bytes(args)), args);
}

FFI(render_glyph) {
    if (args_length != 4 * SIZE_OF_INT64) {
        error("FFI render_glyph: expected font, char, x, y");
        return;
    }

    render_glyph(uint64_of_bytes(args), uint64_of_bytes(args + SIZE_OF_INT64),
                 uint64_of_bytes(args + 2 * SIZE_OF_INT64), uint64_of_bytes(args + 3 * SIZE_OF_INT64));
}

// The framebuffer whose pixels are at [address], or NULL
static struct limine_framebuffer* framebuffer_at(uint64_t address) {
    for (size_t i = 0; i < num_framebuffers; i++) {
        if ((uint64_t)framebuffers[i]->address == address) {
            return framebuffers[i];
        }
    }
    return NULL;
}

FFI(scroll_framebuffer) {
    if (args_length != 2 * SIZE_OF_INT64) {
        error("FFI scroll_framebuffer: expected framebuffer address, h");
        return;
    }

    struct limine_framebuffer* fb = framebuffer_at(uint64_of_bytes(args));
    if (fb == NULL) {
        error("FFI scroll_framebuffer: no framebuffer at 0x%lx", uint64_of_bytes(args));
        return;
    }

    fb_scroll(fb, (uint16_t)uint64_of_bytes(args + SIZE_OF_INT64));
}

FFI(clear_framebuffer_area) {
    if (args_length != 5 * SIZE_OF_INT64) {
        error("FFI clear_framebuffer_area: expected framebuffer address, x, y, w, h");
        return;
    }

    struct limine_framebuffer* fb = framebuffer_at(uint64_of_bytes(args));
    if (fb == NULL) {
        error("FFI clear_framebuffer_area: no framebuffer at 0x%lx", uint64_of_bytes(args));
        return;
    }

    fb_clear_area(
        fb, (uint16_t)uint64_of_bytes(args + SIZE_OF_INT64), (uint16_t)uint64_of_bytes(args + 2 * SIZE_OF_INT64),
        (uint16_t)uint64_of_bytes(args + 3 * SIZE_OF_INT64), (uint16_t)uint64_of_bytes(args + 4 * SIZE_OF_INT64));
}
