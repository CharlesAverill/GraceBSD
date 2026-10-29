#ifndef GRACE_FRAMEBUFFER_H
#define GRACE_FRAMEBUFFER_H

#include <limine.h>

void fb_fill(struct limine_framebuffer* fb, const uint32_t color);
void fb_clear(struct limine_framebuffer* fb);
void fb_pattern(struct limine_framebuffer* fb, const uint32_t* pattern, uint16_t w, uint16_t h);

#endif
