#include <limine.h>
#include <stddef.h>

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

// Scale and draw a pattern to the screen. Pixel format is 0xRRGGBB00
void fb_pattern(struct limine_framebuffer *fb, const uint32_t *pattern, uint16_t w, uint16_t h) {
	volatile uint32_t *fb_ptr = fb->address;
	for (size_t y = 0; y < fb->height; y++) {
		for (size_t x = 0; x < fb->width; x++) {
			int patX = (x * w) / fb->width;
			int patY = (y * h) / fb->height;
			int r = (pattern[patY * w + patX] & 0xFF000000) >> 24;
			int g = (pattern[patY * w + patX] & 0x00FF0000) >> 16;
			int b = (pattern[patY * w + patX] & 0x0000FF00) >> 8;
			fb_ptr[y * (fb->pitch / 4) + x] = fb_pixel(fb, r, g, b);
		}
	}
}
