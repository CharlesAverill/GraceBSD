#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include <limits.h>

#include "serial.h"
#include "framebuffer.h"
#include "generated/splash_data.h"
#include "cakeml.h"

// Set the base revision to 6, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(6);

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

static void hcf(void) {
	while (true);;
}

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
    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
		serial_write("No framebuffer detected\n");
        hcf();
    }

    // Print the pattern to every framebuffer.
    for (uint64_t i = 0; i < framebuffer_request.response->framebuffer_count; i++) {
        struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[i];

        // Ensure the framebuffer has 32-bit RGB pixels, the only kind we handle.
        if (framebuffer->memory_model != LIMINE_FRAMEBUFFER_RGB || framebuffer->bpp != 32) {
			serial_write("framebuffer memory model is not 32-bit RGB\n");
            hcf();
        }

        fb_pattern(framebuffer, splash_pixels, SPLASH_WIDTH, SPLASH_HEIGHT);
        
        for(volatile unsigned int i = 0; i < UINT_MAX / 4; i++);
        fb_clear(framebuffer);
    }

	cakeml_run();
	serial_write("done running cakeml\n");

	hcf();
}
