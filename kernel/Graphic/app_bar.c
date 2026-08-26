#include <stdint.h>
#include "app_bar.h"
#include "rgba.h"
#include "framebuffer.h"
#include "2d_renderer.h"
#include "gfx.h"
#include "sdf.h"
#include "ssaa.h"
#include "io.h"

#define SCALE 1
#define CHAR_W (8 * SCALE)
#define CHAR_H (16 * SCALE)
#define GUI_COLS (1280 / CHAR_W)
#define GUI_ROWS (1280 / CHAR_H)

void appbar_init(Canvas *cv, int x, int y, int w, int h, uint32_t color) {
    
}