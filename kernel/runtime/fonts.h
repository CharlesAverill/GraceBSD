#ifndef GRACE_FONTS_H
#define GRACE_FONTS_H

#include <stddef.h>
#include <stdint.h>

typedef enum Font {
    TS_8x8,
    TS_8x14,
    TS_8x16,
    FontEnd
} Font;

const uint32_t* arr_of_Font(Font f);
size_t width_of_Font(Font f);
size_t height_of_Font(Font f);

// Render [c] with font [f] at screen coordinates (x,y)
void render_glyph(Font f, uint8_t c, uint16_t x, uint16_t y);

#endif
