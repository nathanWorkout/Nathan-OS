#include <stdint.h>
#include "gfx.h"
#include "ssaa.h"
#include "sqrt.h"
#include "rgba.h"
#include "wolf.h"

int64_t clamp(int64_t x, int64_t min, int64_t max) {
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

void draw_circle_sdf(Canvas *cv, int xc, int yc, int R, uint32_t color) {
    uint8_t cr = (color >> 16) & 0xFF;
    uint8_t cg = (color >> 8)  & 0xFF;
    uint8_t cb =  color        & 0xFF;

    for (int y = yc - R - 1; y <= yc + R + 1; y++) {
        for (int x = xc - R - 1; x <= xc + R + 1; x++) {
            int64_t dx = (x-xc);
            int64_t dy = (y-yc);


            int64_t distance = sqrt(dx*dx + dy*dy); 
            int64_t d = distance - R * 1000;
            
            int64_t coverage = clamp(500 - d/4, 0, 1000);

            if (coverage <= 0) continue;
            uint64_t index = (y * (cv->pitch / 4)) + x;
            uint32_t bg = cv->address[index];
            uint8_t br = (bg >> 16) & 0xFF;
            uint8_t bg_ = (bg >> 8) & 0xFF;
            uint8_t bb =  bg        & 0xFF;
            uint8_t r = (uint8_t)((cr * coverage + br * (1000 - coverage)) / 1000);
            uint8_t g = (uint8_t)((cg * coverage + bg_ * (1000 - coverage)) / 1000);
            uint8_t b = (uint8_t)((cb * coverage + bb * (1000 - coverage)) / 1000);

            put_pixel(cv, x, y, rgba(r, g, b, 0xFF));
        }
    }
}

void draw_trait(Canvas *cv, int px, int py, int64_t dist, int64_t thickness, uint32_t color) {
    int64_t coverage = clamp(1000 - (dist - thickness) * 1000 / thickness, 0, 1000);
    if (coverage <= 0) return;

    uint8_t cr = (color >> 16) & 0xFF;
    uint8_t cg = (color >> 8)  & 0xFF;
    uint8_t cb =  color        & 0xFF;

    uint32_t bg = cv->address[py * (cv->pitch / 4) + px];
    uint8_t br = (bg >> 16) & 0xFF;
    uint8_t bg_ = (bg >> 8) & 0xFF;
    uint8_t bb =  bg        & 0xFF;

    uint8_t r = (cr * coverage + br * (1000 - coverage)) / 1000;
    uint8_t g = (cg * coverage + bg_ * (1000 - coverage)) / 1000;
    uint8_t b = (cb * coverage + bb * (1000 - coverage)) / 1000;

    put_pixel(cv, px, py, rgba(r, g, b, 0xFF));
}

