#define FONT8x16_IMPLEMENTATION
#include "font.h"
#include "framebuffer.h"
#include "gfx.h"
#include "rgba.h"
#include <stdint.h>

void draw_char(Canvas *cv, char c, int x, int y, uint32_t color, int scale) {
    unsigned char *data = font8x16[(int)c];
    
    for(int i = 0; i < 16; i++)
        for(int j = 0; j < 8; j++)
            if(data[i] & (0x80 >> j))
                for(int sy = 0; sy < scale; sy++)
                    for(int sx = 0; sx < scale; sx++)
                        put_pixel(cv, x + j*scale + sx, y + i*scale + sy, color);
}

void draw_string(Canvas *cv, const char *str, int x, int y, uint32_t color, int scale) {
    while (*str) {
        draw_char(cv, *str, x, y, color, scale);
        x += 8 * scale;
        str++;
    }
}