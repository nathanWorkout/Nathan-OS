#ifndef SDF_H
#define SDF_H
#include <stdint.h>
#include "gfx.h"

int64_t clamp(int64_t x, int64_t min, int64_t max);
void draw_circle_sdf(Canvas *cv, int xc, int yc, int R, uint32_t color);
void draw_trait(Canvas *cv, int px, int py, int64_t dist, int64_t thickness, uint32_t color);
void draw_error_screen(Canvas *cv);

#endif