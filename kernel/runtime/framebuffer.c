#include <limine.h>
#include <stddef.h>

#include "serial.h"

// Scale an 8-bit colour channel value to the size the framebuffer gives the
// channel and move it into place within a pixel.
uint32_t fb_channel(uint8_t value, uint8_t mask_size, uint8_t mask_shift) {
    uint64_t max = ((uint64_t)1 << mask_size) - 1;
    return (uint32_t)((value * max / 255) << mask_shift);
}

// Build a pixel from 8-bit red, green and blue values following the channel
// layout of the framebuffer.
uint32_t fb_pixel(struct limine_framebuffer *fb, uint8_t red, uint8_t green, uint8_t blue) {
    return fb_channel(red, fb->red_mask_size, fb->red_mask_shift)
         | fb_channel(green, fb->green_mask_size, fb->green_mask_shift)
         | fb_channel(blue, fb->blue_mask_size, fb->blue_mask_shift);
}

#define RED(x)   ((x & 0xFF000000) >> 24)
#define GREEN(x) ((x & 0x00FF0000) >> 16)
#define BLUE(x)  ((x & 0x0000FF00) >> 8)

void fb_fill(struct limine_framebuffer *fb, const uint32_t color) {
    volatile uint32_t *fb_ptr=fb->address;
    for (size_t y = 0; y < fb->height; y++) {
        for (size_t x = 0; x < fb->width; x++) {
            fb_ptr[y * (fb->pitch / 4) + x] = fb_pixel(fb, RED(color), GREEN(color), BLUE(color));
        }
    }
}

void fb_clear(struct limine_framebuffer *fb) {
    fb_fill(fb, 0);
}

// Scale and draw a pattern to the screen. Pixel format is 0xRRGGBB00
void fb_pattern(struct limine_framebuffer *fb, const uint32_t *pattern, uint16_t w, uint16_t h) {
	volatile uint32_t *fb_ptr = fb->address;

    // Resample image down if it is larger than the framebuffer
	if (fb->height < h || fb->width < w) {
        serial_write("Downsampling splash\n");
		for (size_t y = 0; y < fb->height; y++) {
            for (size_t x = 0; x < fb->width; x++) {
                int patX = (x * w) / fb->width;
                int patY = (y * h) / fb->height;
                fb_ptr[y * (fb->pitch / 4) + x] = fb_pixel(fb, RED(pattern[patY * w + patX]),
                                                           GREEN(pattern[patY * w + patX]),
                                                           BLUE(pattern[patY * w + patX]));
            }
        }

        return;
    }

	// Draw top-left color to entire framebuffer
    serial_write("Filling framebuffer\n");
    fb_fill(fb, pattern[0]);
	
	// Draw splash, centered
    serial_write("Drawing splash\n");
	int startX = (fb->width - w) / 2;
	int startY = (fb->height - h)  / 2;
	for (size_t y = 0; y < h; y++) {
		for (size_t x = 0; x < w; x++) {
			int off = y * w + x;
			fb_ptr[(y + startY) * (fb->pitch / 4) + (x + startX)] =
				fb_pixel(fb, RED(pattern[off]),
					     GREEN(pattern[off]),
					     BLUE(pattern[off]));
		}
	}
}
