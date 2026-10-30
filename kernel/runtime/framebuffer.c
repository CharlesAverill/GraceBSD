#include <limine.h>
#include <stddef.h>

#include "logging.h"

// Scale an 8-bit colour channel value to the size the framebuffer gives the
// channel and move it into place within a pixel.
uint32_t fb_channel(uint8_t value, uint8_t mask_size, uint8_t mask_shift) {
    uint64_t max = ((uint64_t)1 << mask_size) - 1;
    return (uint32_t)((value * max / 255) << mask_shift);
}

// Build a pixel from 8-bit red, green and blue values following the channel
// layout of the framebuffer.
uint32_t fb_pixel(struct limine_framebuffer* fb, uint8_t red, uint8_t green, uint8_t blue) {
    return fb_channel(red, fb->red_mask_size, fb->red_mask_shift) |
           fb_channel(green, fb->green_mask_size, fb->green_mask_shift) |
           fb_channel(blue, fb->blue_mask_size, fb->blue_mask_shift);
}

#define RED(x)   (((x)&0xFF000000) >> 24)
#define GREEN(x) (((x)&0x00FF0000) >> 16)
#define BLUE(x)  (((x)&0x0000FF00) >> 8)

// Convert a 0xRRGGBB00 colour to the framebuffer's pixel format.
static uint32_t fb_color(struct limine_framebuffer* fb, uint32_t color) {
    return fb_pixel(fb, RED(color), GREEN(color), BLUE(color));
}

void fb_fill(struct limine_framebuffer* fb, const uint32_t color) {
    volatile uint32_t* fb_ptr = fb->address;
    uint32_t pixel            = fb_color(fb, color);
    for (size_t y = 0; y < fb->height; y++) {
        for (size_t x = 0; x < fb->width; x++) {
            fb_ptr[y * (fb->pitch / 4) + x] = pixel;
        }
    }
}

void fb_clear(struct limine_framebuffer* fb) {
    fb_fill(fb, 0);
}

// Scale and draw a pattern to the screen, centered. Pixel format is 0xRRGGBB00
void fb_pattern(struct limine_framebuffer* fb, const uint32_t* pattern, uint16_t w, uint16_t h) {
    volatile uint32_t* fb_ptr = fb->address;

    // Shrink the pattern to fit if it is larger than the framebuffer, keeping
    // its aspect ratio by scaling both axes by the smaller of the two ratios
    size_t out_w = w;
    size_t out_h = h;
    if (fb->width < out_w || fb->height < out_h) {
        debug("Downsampling splash");
        if ((uint64_t)fb->width * h < (uint64_t)fb->height * w) {
            out_w = fb->width;
            out_h = (size_t)((uint64_t)h * fb->width / w);
        } else {
            out_h = fb->height;
            out_w = (size_t)((uint64_t)w * fb->height / h);
        }
    }

    // Draw top-left color to entire framebuffer
    debug("Filling framebuffer");
    fb_fill(fb, pattern[0]);

    // Draw splash, centered
    debug("Drawing splash");
    size_t start_x = (fb->width - out_w) / 2;
    size_t start_y = (fb->height - out_h) / 2;
    for (size_t y = 0; y < out_h; y++) {
        size_t pat_y = y * h / out_h;
        for (size_t x = 0; x < out_w; x++) {
            size_t pat_x                                            = x * w / out_w;
            fb_ptr[(y + start_y) * (fb->pitch / 4) + (x + start_x)] = fb_color(fb, pattern[pat_y * w + pat_x]);
        }
    }
}
