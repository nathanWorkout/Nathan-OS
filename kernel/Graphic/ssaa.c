#include <stdint.h>
#include "gfx.h"
#include "ssaa.h"
#include "sqrt.h"
#include "rgba.h"

static int64_t samples[4][2] = {
    {250, 250},
    {750, 250},
    {250, 750},
    {750, 750},
};

void draw_circle_ssaa(Canvas *cv, int xc, int yc, int R, uint32_t color) {
    int64_t r2 = (int64_t)R * R * 1000000;
    uint8_t cr = (color >> 16) & 0xFF;
    uint8_t cg = (color >> 8)  & 0xFF;
    uint8_t cb =  color        & 0xFF;

    for (int y = yc - R; y <= yc + R; y++) {
        for (int x = xc - R; x <= xc + R; x++) {
            int nb_samples = 0;
            for (int s = 0; s < 4; s++) {
                int64_t sx = (x * 1000 + samples[s][0]) - xc * 1000;
                int64_t sy = (y * 1000 + samples[s][1]) - yc * 1000;
                if ((sx * sx + sy * sy) <= r2)
                    nb_samples++;
            }

            if (nb_samples == 0) {
                continue;
            }
            
            int64_t coverage = nb_samples * 1000 / 4;
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