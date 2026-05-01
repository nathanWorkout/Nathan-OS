#ifndef GFX_H  
#define GFX_H  
#include <stdint.h>

typedef struct {
    uint32_t *address;
    uint64_t width;
    uint64_t height;
    uint64_t pitch;    
} Canvas;

void put_pixel(Canvas *cv, uint64_t x, uint64_t y, uint32_t color);
void color_screen(Canvas *cv, uint32_t color);
void clear_screen(Canvas *cv); 
void gfx_init(Canvas *cv);


#endif
