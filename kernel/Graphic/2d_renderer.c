#include <stdint.h>
#include "gfx.h"
#include "rgba.h"

void draw_circle(Canvas *cv, int xc, int yc, int R, uint32_t color) {
    int r2 = R * R;

    for (int y = yc - R; y <= yc + R; y++) {
        for (int x = xc - R; x <= xc + R; x++) {
            int dx = x - xc;
            int dy = y - yc;

            if ((dx * dx + dy * dy) <= r2) {
                put_pixel(cv, x, y, color);
            }
        }
    }
}

void draw_rectangle(Canvas *cv, int x, int y, int w, int h, uint32_t color) {
    for (int py = y; py < y + h; py++) {
        for (int px = x; px < x + w; px++) {
            put_pixel(cv, px, py, color);
        }
    }
}


