#include <stdint.h>
#include "gfx.h"
#include "ssaa.h"

static float samples[4][2] = {
    {0.25f, 0.25f},
    {0.75f, 0.25f},
    {0.25f, 0.75f},
    {0.75f, 0.75f},
};

void draw_circle_ssaa(Canvas *cv, int xc, int yc, int R, uint32_t color) {
    int r2 = R * R;
    uint8_t cr = (color >> 16) & 0xFF;
    uint8_t cg = (color >> 8)  & 0xFF;
    uint8_t cb =  color        & 0xFF;

    for (int y = yc - R; y <= yc + R; y++) {
        for (int x = xc - R; x <= xc + R; x++) {
            int hits = 0;
            for (int s = 0; s < 4; s++) {
                float sx = (x + samples[s][0]) - xc;
                float sy = (y + samples[s][1]) - yc;
                if ((sx * sx + sy * sy) <= r2)
                    hits++;
            }
            if (hits == 0) continue;

            float coverage = hits / 4.0f;

            uint64_t index = (y * (cv->pitch / 4)) + x;
            uint32_t bg = cv->address[index];
            uint8_t br = (bg >> 16) & 0xFF;
            uint8_t bg_ = (bg >> 8) & 0xFF;
            uint8_t bb =  bg        & 0xFF;

            uint8_t r = (uint8_t)(cr * coverage + br * (1.0f - coverage));
            uint8_t g = (uint8_t)(cg * coverage + bg_ * (1.0f - coverage));
            uint8_t b = (uint8_t)(cb * coverage + bb * (1.0f - coverage));

            put_pixel(cv, x, y, rgba(r, g, b, 0xFF));
        }
    }
}
