#include "fonts.h"
#include "framebuffer.h"
#include "hw.h"

#include "generated/font_Bm437_ToshibaSat_8x14.h"
#include "generated/font_Bm437_ToshibaSat_8x16.h"
#include "generated/font_Bm437_ToshibaSat_8x8.h"

const uint32_t* arr_of_Font(Font f) {
    switch (f) {
    case TS_8x8:
        return &font_bm437_toshibasat_8x8[0][0];
    case TS_8x14:
        return &font_bm437_toshibasat_8x14[0][0];
    case TS_8x16:
        return &font_bm437_toshibasat_8x16[0][0];
    case FontEnd:
        break;
    }

    return NULL;
}

size_t width_of_Font(Font f) {
    switch (f) {
    case TS_8x8:
        return FONT_BM437_TOSHIBASAT_8X8_WIDTH;
    case TS_8x14:
        return FONT_BM437_TOSHIBASAT_8X14_WIDTH;
    case TS_8x16:
        return FONT_BM437_TOSHIBASAT_8X16_WIDTH;
    case FontEnd:
        break;
    }

    return 0;
}

size_t height_of_Font(Font f) {
    switch (f) {
    case TS_8x8:
        return FONT_BM437_TOSHIBASAT_8X8_HEIGHT;
    case TS_8x14:
        return FONT_BM437_TOSHIBASAT_8X14_HEIGHT;
    case TS_8x16:
        return FONT_BM437_TOSHIBASAT_8X16_HEIGHT;
    case FontEnd:
        break;
    }

    return 0;
}

void render_glyph(Font f, uint8_t c, uint16_t x, uint16_t y) {
    const uint32_t* glyphs = arr_of_Font(f);
    // The fonts have one glyph per ASCII code
    if (glyphs == NULL || c >= 128) {
        return;
    }

    size_t w = width_of_Font(f);
    size_t h = height_of_Font(f);
    fb_glyph(FB_MAIN, glyphs + c * w * h, x, y, w, h);
}
