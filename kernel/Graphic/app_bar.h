#ifndef APP_BAR_H
#define APP_BAR_H
#include <stdint.h>
#include "framebuffer.h"

void appbar_init(Canvas *cv, int x, int y, int w, int h, uint32_t color);

#endif