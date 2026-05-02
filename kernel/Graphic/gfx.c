#include <stdint.h>
#include "com1.h"
#include "gfx.h"
#include "2d_renderer.h"
#include "ssaa.h"
#include "sdf.h"
#include "rgba.h"
#include "wolf.h"
#include "font.h"
#include "kernel_panic.h"

void put_pixel(Canvas *cv, uint64_t x, uint64_t y, uint32_t color) {
    if (x >= cv->width || y >= cv->height) return;
    uint64_t index = (y * (cv->pitch / 4)) + x;
    cv->address[index] = color;
}

void color_screen(Canvas *cv, uint32_t color) {
    uint64_t stride = cv->pitch / 4;
    for (uint64_t y = 0; y < cv->height; y++) {
        uint32_t *row = cv->address + y * stride;   
        for (uint64_t x = 0; x < cv->width; x++)
            row[x] = color;
    }
}

void clear_screen(Canvas *cv) {
    color_screen(cv, 0x00000000);
}

void gfx_init(Canvas *cv) {
    
}
