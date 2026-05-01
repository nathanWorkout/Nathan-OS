#include <stdint.h>
#include "gfx.h"
#include "ssaa.h"
#include "sqrt.h"
#include "rgba.h"

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
            
            int64_t coverage = clamp(1000 - d, 0, 1000);

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

void draw_error_screen(Canvas *cv) {
    int ox = cv->width / 4;
    int oy = cv->height / 2;
    int64_t scale = 110;
    int64_t thickness = 18;
    uint32_t color = rgba(0, 0, 0, 255);

    for (int py = 0; py < cv->height; py++) {
        for (int px = 0; px < cv->width; px++) {
            int64_t x = (int64_t)(px - ox) * 1000 / scale;
            int64_t y = (int64_t)(oy - py) * 1000 / scale;
            int64_t x2 = x * x / 1000;
            int64_t y2 = y * y / 1000;
            int64_t min_d = 999999;
            int64_t d;

            if (x > 200 && x < 1200) {
                int64_t fy = (-1330 * x2 + 243 * x + 506000) / 1000;
                d = y - fy; if (d < 0) d = -d;
                if (d < min_d) min_d = d;
            }

            if (y > 500 && y < 1000) {
                int64_t fx = (1600 * y2 - 2220 * y + 920000) / 1000;
                d = x - fx; if (d < 0) d = -d;
                if (d < min_d) min_d = d;
            }

            if (x > 300 && x < 700) {
                d = y - 1000; if (d < 0) d = -d;
                if (d < min_d) min_d = d;
            }

            if (x > -300 && x < 700) {
                int64_t fy = (-1375 * x2 - 850 * x + 2269000) / 1000;
                d = y - fy; if (d < 0) d = -d;
                if (d < min_d) min_d = d;
            }

            if (x >= 1100 && x <= 1180) {
                int64_t t  = (x - 1100) * 1000 / 80;
                int64_t t2 = t * t / 1000;
                int64_t t3 = t2 * t / 1000;
                int64_t h00 = 2*t3 - 3*t2 + 1000;
                int64_t h10 = t3 - 2*t2 + t;
                int64_t h01 = -2*t3 + 3*t2;
                int64_t h11 = t3 - t2;
                int64_t fy = (h00*(-1750) + h10*(875) + h01*(-1100) + h11*(1458)) / 1000000;
                d = y - fy; if (d < 0) d = -d;
                if (d < min_d) min_d = d;
            }

            if (y >= 2000 && y <= 2400) {
                d = x - (-300); if (d < 0) d = -d;
                if (d < min_d) min_d = d;
            }

            if (y >= 1000 && y <= 2000) {
                d = x - (-500); if (d < 0) d = -d;
                if (d < min_d) min_d = d;
            }

            if (y >= -500 && y <= 1000) {
                int64_t fx = (y2 - 500 * y / 1000 - 1000000) / 1000;
                d = x - fx; if (d < 0) d = -d;
                if (d < min_d) min_d = d;
            }

            if (y >= -1500 && y <= -500) {
                d = x - (-500); if (d < 0) d = -d;
                if (d < min_d) min_d = d;
            }

            if (y >= -1800 && y <= -1500) {
                int64_t fx = (y + 1500) / 3 - 500;
                d = x - fx; if (d < 0) d = -d;
                if (d < min_d) min_d = d;
            }

            if (y >= -1800 && y <= -1000) {
                int64_t fx = -(y + 1000);
                d = x - fx; if (d < 0) d = -d;
                if (d < min_d) min_d = d;
            }

            draw_trait(cv, px, py, min_d, thickness, color);
        }
    }
}