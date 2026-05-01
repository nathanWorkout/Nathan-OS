#ifndef RENDERER_2D_H
#define RENDERER_2D_H

#include <stdint.h>
#include "gfx.h"

void draw_circle(Canvas *cv, int xc, int yc, int R, uint32_t color);
void draw_rectangle(Canvas *cv, int x, int y, int w, int h, uint32_t color);

#endif
