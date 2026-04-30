#ifndef GFX_H  
#define GFX_H  
#include <stdint.h>

static inline uint32_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

typedef struct {
    uint32_t *address;
    uint64_t width;
    uint64_t height;
    uint64_t pitch;    
} Canvas;

void put_pixel(Canvas *cv, uint64_t x, uint64_t y, uint32_t color);
void color_screen(Canvas *cv, uint32_t color);
void clear_screen(Canvas *cv); 


#endif
