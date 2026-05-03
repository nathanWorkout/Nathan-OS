#ifndef COLOR_H
#define COLOR_H
#include <stdint.h>

#define RGB(color) rgba((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, 0xFF)

static inline uint32_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    (void)a;
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

#endif