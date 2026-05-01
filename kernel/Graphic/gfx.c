#include <stdint.h>
#include "com1.h"
#include "gfx.h"
#include "2d_renderer.h"
#include "ssaa.h"
#include "sdf.h"
#include "rgba.h"

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
    color_screen(cv, rgba(10, 10, 35, 255));
    
    int lune_x = cv->width / 4;
    int lune_y = cv->height / 2;
    int lune_r = cv->height / 6;
    draw_circle_sdf(cv, lune_x, lune_y, lune_r, rgba(255, 220, 50, 255));

    int sol_y = cv->height * 21 / 25;
    int sol_h = cv->height - sol_y;
    draw_rectangle(cv, 0, sol_y, cv->width, sol_h, rgba(20, 80, 20, 255));
    draw_error_screen(cv);
}
