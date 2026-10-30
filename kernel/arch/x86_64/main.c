#include <limine.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "cakeml.h"
#include "framebuffer.h"
#include "generated/splash_data.h"
#include "serial.h"

// Set the base revision to 6, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

__attribute__((used, section(".limine_requests"))) static volatile uint64_t limine_base_revision[] =
    LIMINE_BASE_REVISION(6);

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.

__attribute__((used,
               section(".limine_requests"))) static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID, .revision = 0
};

// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.

__attribute__((used, section(".limine_requests_start"))) static volatile uint64_t limine_requests_start_marker[] =
    LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end"))) static volatile uint64_t limine_requests_end_marker[] =
    LIMINE_REQUESTS_END_MARKER;

// Request memory map
__attribute__((used, section(".limine_requests"))) static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID, .revision = 0
};

// Request HHDM (Higher-Half Direct Map)
// Dereferencing physical RAM addresses directly will cause page faults because Limine turns on paging
// To dereference a physical RAM address, you have to add the HHDM offset to the pointer
__attribute__((used, section(".limine_requests"))) static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID, .revision = 0
};

static void hcf(void) {
    for (;;) {
        __asm__ volatile("hlt");
    }
}

uint64_t hhdm_off = 0;

// Kernel entrypoint, pointed to by /boot/limine.conf
void kmain(void) {
    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        hcf();
    }

    // Set up the UART console
    serial_init();
    serial_write("Hello, World from GraceBSD!\n");

    // Ensure we got a framebuffer.
    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1) {
        serial_write("No framebuffer detected\n");
        hcf();
    }

    // Print the pattern to every framebuffer.
    for (uint64_t i = 0; i < framebuffer_request.response->framebuffer_count; i++) {
        struct limine_framebuffer* framebuffer = framebuffer_request.response->framebuffers[i];

        // Ensure the framebuffer has 32-bit RGB pixels, the only kind we handle.
        if (framebuffer->memory_model != LIMINE_FRAMEBUFFER_RGB || framebuffer->bpp != 32) {
            serial_write("framebuffer memory model is not 32-bit RGB\n");
            hcf();
        }

        fb_pattern(framebuffer, splash_pixels, SPLASH_WIDTH, SPLASH_HEIGHT);

        // for (volatile unsigned int i = 0; i < UINT_MAX / 4; i++) { }
        // fb_clear(framebuffer);
    }

    // Print out memory map entries and select the largest one to use as kernel
    // heap and eventually user process memory
    char* largest_off;
    uint64_t largest_len = 0;
    serial_write("== MEMORY MAP ==\n");
    if (memmap_request.response == NULL) {
        serial_write("FAILURE: No memory map response\n");
        hcf();
    }
    for (uint64_t i = 0; i < memmap_request.response->entry_count; i++) {
        struct limine_memmap_entry* entry = memmap_request.response->entries[i];

        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= CAKEML_HEAP_SIZE + CAKEML_STACK_SIZE) {
            largest_off = largest_len < entry->length ? (char*)entry->base : largest_off;
            largest_len = largest_len < entry->length ? entry->length : largest_len;
        }

        serial_write_int(i);
        serial_write(": BASE=");
        serial_write_int(entry->base);
        serial_write("; LENGTH=");
        serial_write_int(entry->length);
        serial_write("; TYPE=");
        switch (entry->type) {
        case LIMINE_MEMMAP_USABLE:
            serial_write("USABLE\n");
            break;
        case LIMINE_MEMMAP_RESERVED:
            serial_write("MEMMAP_RESERVED\n");
            break;
        case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
            serial_write("MEMMAP_ACPI_RECLAIMABLE\n");
            break;
        case LIMINE_MEMMAP_ACPI_NVS:
            serial_write("MEMMAP_ACPI_NVS\n");
            break;
        case LIMINE_MEMMAP_BAD_MEMORY:
            serial_write("MEMMAP_BAD_MEMORY\n");
            break;
        case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
            serial_write("MEMMAP_BOOTLOADER_RECLAIMABLE\n");
            break;
        case LIMINE_MEMMAP_EXECUTABLE_AND_MODULES:
            serial_write("MEMMAP_EXECUTABLE_AND_MODULES\n");
            break;
        case LIMINE_MEMMAP_FRAMEBUFFER:
            serial_write("MEMMAP_FRAMEBUFFER\n");
            break;
        case LIMINE_MEMMAP_RESERVED_MAPPED:
            serial_write("MEMMAP_RESERVED_MAPPED\n");
            break;
        }
    }

    // Report on found memory segment
    if (largest_len < CAKEML_HEAP_SIZE + CAKEML_STACK_SIZE) {
        serial_write("FAILURE: Did not find a suitable memory region for the kernel\n");
        hcf();
    }
    serial_write("Using segment at ");
    serial_write_int((int64_t)largest_off);
    serial_write(" with length ");
    serial_write_int(largest_len);
    serial_write("\n");

    // Get HHDM offset for [largest_off]
    if (hhdm_request.response == NULL) {
        serial_write("FAILURE: No HHDM response\n");
        hcf();
    }
    hhdm_off = hhdm_request.response->offset;

    cakeml_run(hhdm_off + largest_off, largest_len);
    serial_write("done running cakeml\n");

    hcf();
}
