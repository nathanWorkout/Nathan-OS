#include <stdint.h>
#include "gfx.h"

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


