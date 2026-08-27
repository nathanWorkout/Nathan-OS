#include <stdint.h>
#include <stddef.h>
#include "app_bar.h"
#include "rgba.h"
#include "framebuffer.h"
#include "2d_renderer.h"
#include "tty.h"
#include "gfx.h"
#include "sdf.h"
#include "ssaa.h"
#include "io.h"

#define SCALE 1
#define CHAR_W (8 * SCALE)
#define CHAR_H (16 * SCALE)
#define GUI_COLS (1280 / CHAR_W)
#define GUI_ROWS (1280 / CHAR_H)

#include "wallpaper_loader/png.h"

void default_profile_init(Canvas cv, PngContext wallpaper) {
    tty_clear();

    if (wallpaper.pixels != NULL) {
        uint32_t bytes_per_pixel;
        switch (wallpaper.color_type) {
            case PNG_COLOR_RGB:  bytes_per_pixel = 3; break;
            case PNG_COLOR_RGBA: bytes_per_pixel = 4; break;
            default:             bytes_per_pixel = 3; break;
        }

        for (uint32_t y = 0; y < wallpaper.height && y < (uint32_t)cv.height; y++) {
            uint32_t row_start = y * (wallpaper.width * bytes_per_pixel + 1) + 1;
            for (uint32_t x = 0; x < wallpaper.width && x < (uint32_t)cv.width; x++) {
                uint32_t idx = row_start + x * bytes_per_pixel;
                uint8_t r = wallpaper.pixels[idx];
                uint8_t g = wallpaper.pixels[idx + 1];
                uint8_t b = wallpaper.pixels[idx + 2];
                uint8_t a = (bytes_per_pixel == 4) ? wallpaper.pixels[idx + 3] : 255;
                put_pixel(&cv, x, y, rgba(r, g, b, a));
            }
        }
    } else {
        color_screen(&cv, rgba(13, 137, 203, 255));
    }

    while (1) {
        __asm__ volatile("hlt");
    }
}