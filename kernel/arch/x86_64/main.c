#include <limine.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "cakeml.h"
#include "framebuffer.h"
#include "generated/splash_data.h"
#include "hw.h"
#include "logging.h"
#include "serial.h"
#include "tables/gdt.h"
#include "tables/idt.h"

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

void hcf(void) {
    for (;;) {
        __asm__ volatile("hlt");
    }
}

uint64_t hhdm_off                                     = 0;
LIMINE_PTR(struct limine_memmap_entry**) mmap_entries = NULL;
size_t num_mmap_entries                               = 0;
struct limine_framebuffer** framebuffers              = NULL;
size_t num_framebuffers                               = 0;

static const char* memmap_type_name(uint64_t type) {
    switch (type) {
    case LIMINE_MEMMAP_USABLE:
        return "USABLE";
    case LIMINE_MEMMAP_RESERVED:
        return "RESERVED";
    case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
        return "ACPI_RECLAIMABLE";
    case LIMINE_MEMMAP_ACPI_NVS:
        return "ACPI_NVS";
    case LIMINE_MEMMAP_BAD_MEMORY:
        return "BAD_MEMORY";
    case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
        return "BOOTLOADER_RECLAIMABLE";
    case LIMINE_MEMMAP_EXECUTABLE_AND_MODULES:
        return "EXECUTABLE_AND_MODULES";
    case LIMINE_MEMMAP_FRAMEBUFFER:
        return "FRAMEBUFFER";
    case LIMINE_MEMMAP_RESERVED_MAPPED:
        return "RESERVED_MAPPED";
    default:
        return "UNKNOWN";
    }
}

// Kernel entrypoint, pointed to by /boot/limine.conf
void kmain(void) {
    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        hcf();
    }

    // Set up the UART console
    serial_init();
    info("Hello, World from GraceBSD!");

    // Set up the Global Descriptor Table
    load_gdt();
    // Set up the Interrupt Descriptor Table
    idt_init();

    // Ensure we got a framebuffer.
    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1) {
        fatal(RC_INIT_ERROR, "No framebuffer detected");
    }

    // Print the pattern to every framebuffer.
    framebuffers     = framebuffer_request.response->framebuffers;
    num_framebuffers = framebuffer_request.response->framebuffer_count;
    for (uint64_t i = 0; i < num_framebuffers; i++) {
        struct limine_framebuffer* framebuffer = framebuffers[i];

        // Ensure the framebuffer has 32-bit RGB pixels, the only kind we handle.
        if (framebuffer->memory_model != LIMINE_FRAMEBUFFER_RGB || framebuffer->bpp != 32) {
            fatal(RC_INIT_ERROR, "Framebuffer memory model is not 32-bit RGB");
        }

        fb_pattern(framebuffer, splash_pixels, SPLASH_WIDTH, SPLASH_HEIGHT);

        for (volatile unsigned int i = 0; i < UINT_MAX / 8; i++) { }
        fb_clear(framebuffer);
    }

    // Print out memory map entries and select the largest one to use as kernel
    // heap and eventually user process memory
    uint8_t* largest_off = NULL;
    uint64_t largest_len = 0;
    if (memmap_request.response == NULL) {
        fatal(RC_INIT_ERROR, "No memory map response");
    }
    debug("Memory map:");
    debug("%3s  %-18s  %-18s  %s", "#", "BASE", "LENGTH", "TYPE");
    mmap_entries     = memmap_request.response->entries;
    num_mmap_entries = memmap_request.response->entry_count;
    for (uint64_t i = 0; i < num_mmap_entries; i++) {
        struct limine_memmap_entry* entry = mmap_entries[i];

        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= CAKEML_HEAP_SIZE + CAKEML_STACK_SIZE) {
            largest_off = largest_len < entry->length ? (uint8_t*)entry->base : largest_off;
            largest_len = largest_len < entry->length ? entry->length : largest_len;
        }

        debug("%3lu  0x%016lx  0x%016lx  %s", i, entry->base, entry->length, memmap_type_name(entry->type));
    }

    // Report on found memory segment
    if (largest_len < CAKEML_HEAP_SIZE + CAKEML_STACK_SIZE) {
        fatal(RC_MEMORY_ERROR, "Did not find a suitable memory region for the kernel");
    }
    info("Using memory at 0x%lx with length 0x%lx", (uint64_t)largest_off, largest_len);

    // Get HHDM offset for [largest_off]
    if (hhdm_request.response == NULL) {
        fatal(RC_INIT_ERROR, "No HHDM response");
    }
    hhdm_off = hhdm_request.response->offset;

    cakeml_run(hhdm_off + largest_off, largest_len);
    info("Done running CakeML");

    hcf();
}
