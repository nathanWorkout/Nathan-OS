#include <stdint.h>
#include <stdarg.h>
#include "com1.h"
#include "gfx.h"
#include "framebuffer.h"
#include "font.h"
#include "rgba.h"
#include "2d_renderer.h"
#include "io.h"

#define SCALE 1
#define CHAR_W (8 * SCALE)
#define CHAR_H (16 * SCALE)
#define TTY_COLS (1280 / CHAR_W)
#define TTY_ROWS (720  / CHAR_H)

static Canvas cv;
static int cursor_x = 0;
static int cursor_y = 0;
static uint32_t color = 0xFFFFFFFF; 


void tty_init(Canvas c) {
    cv = c;
    cursor_x = 0;
    cursor_y = 0;
    // désactive le curseur VGA hardware
    outb(0x3d4, 0x0a);
    outb(0x3d5, 0x20);

    color_screen(&cv, 0x00000000);
}

void putchar(char c) {
    switch(c) {
        case '\n': cursor_y++; cursor_x = 0; break;
        case '\r': cursor_x = 0; break;
        case '\t': cursor_x += 4; break;
        case '\b':
        if (cursor_x > 0) {
            cursor_x--;
            draw_rectangle(&cv, cursor_x * CHAR_W, cursor_y * CHAR_H, CHAR_W, CHAR_H, 0x000000);
        }
        break;
			
        default:
            draw_char(&cv, c, cursor_x * CHAR_W, cursor_y * CHAR_H, color, SCALE);
            cursor_x++;
            break;
    }

    if(cursor_x >= TTY_COLS) {
        cursor_x = 0;
        cursor_y++;
    }

    if(cursor_y >= TTY_ROWS) {
        for(int y = 1; y < TTY_ROWS; y++) {
            for(int x = 0; x < TTY_COLS; x++) {
                for(int py = 0; py < CHAR_H; py++) {
                    for(int px = 0; px < CHAR_W; px++) {
                        uint32_t pixel = cv.address[(y * CHAR_H + py) * (cv.pitch / 4) + x * CHAR_W + px];
                        cv.address[((y-1) * CHAR_H + py) * (cv.pitch / 4) + x * CHAR_W + px] = pixel;
                    }
                }
            }
        }
        draw_rectangle(&cv, 0, (TTY_ROWS - 1) * CHAR_H, cv.width, CHAR_H, 0x000000);
        cursor_y = TTY_ROWS - 1;
        cursor_x = 0;
    }
}

void puts(char *s) {
    while(*s != '\0') {
        putchar(*s);
        s++;
    }
    putchar('\n');
}

int printk(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int count = 0;

    while(*fmt) {
        if(*fmt == '%') {
            fmt++;
            switch(*fmt) {
                case 'c': {
                    char c = (char)va_arg(args, int);
                    putchar(c);
                    count++;
                    break;
                }
                case 's': {
                    char *s = va_arg(args, char*);
                    while(*s) { putchar(*s++); count++; }
                    break;
                }
                case 'd': {
                    char buffer[20];
                    int i = 19;
                    int n = va_arg(args, int);
                    if(n == 0) { buffer[i--] = '0'; }
                    while(n > 0) { buffer[i--] = '0' + (n % 10); n /= 10; }
                    while(++i < 20) { putchar(buffer[i]); count++; }
                    break;
                }
                case 'x': {
                    char buf[19];
                    buf[0] = '0'; buf[1] = 'x';
                    const char hex[] = "0123456789ABCDEF";
                    uint64_t n = va_arg(args, uint64_t);
                    for(int i = 15; i >= 0; i--) { buf[2+i] = hex[n & 0xF]; n >>= 4; }
                    buf[18] = '\0';
                    for(int i = 0; buf[i]; i++) { putchar(buf[i]); count++; }
                    break;
                }
            }
            fmt++;
        } else {
            putchar(*fmt);
            fmt++;
            count++;
        }
    }

    va_end(args);
    return count;
}

void tty_set_color(uint32_t c) {
    color = c;
}
void tty_clear() {
    color_screen(&cv, 0x00000000);
    cursor_x = 0;
    cursor_y = 0;
}

void tty_reboot() {
    // Méthode 1 
    uint8_t val = inb(0x64);
    while (val & 0x02) val = inb(0x64); 
    outb(0x64, 0xFE);
    
    // Méthode 2 : triple fault 
    __asm__ volatile (
        "lidt 0\n"
        "int $3\n"
    );
    
    // Méthode 3 : boucle infinie au pire
    while(1) __asm__ volatile("hlt");
}

void tty_draw_cursor(int visible) {
    uint32_t col = visible ? color : 0x000000;
    draw_rectangle(&cv, cursor_x * CHAR_W, cursor_y * CHAR_H, CHAR_W, CHAR_H, col);
}
