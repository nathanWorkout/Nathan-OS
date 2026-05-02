#include <stdint.h>
#include "gfx.h"
#include "2d_renderer.h"
#include "ssaa.h"
#include "sdf.h"
#include "rgba.h"
#include "wolf.h"
#include "font.h"
#include "2d_renderer.h"
#include "isr.h"
#include "idt.h"
#include "com1.h"
extern void irq0();
extern void irq1();


static void draw_hex(Canvas *cv, int x, int y, uint64_t val, uint32_t color) {
    char buf[19];
    buf[0] = '0'; buf[1] = 'x';
    const char hex[] = "0123456789ABCDEF";
    for (int i = 15; i >= 0; i--) {
        buf[2 + i] = hex[val & 0xF];
        val >>= 4;
    }
    buf[18] = '\0';
    draw_string(cv, buf, x, y, color, 1);
}

static void draw_dec(Canvas *cv, int x, int y, uint64_t val, uint32_t color) {
    char buf[12];
    int i = 10;
    buf[11] = '\0';
    if (val == 0) { buf[i--] = '0'; }
    else { while (val > 0) { buf[i--] = '0' + (val % 10); val /= 10; } }
    draw_string(cv, buf + i + 1, x, y, color, 2);
}

void draw_error_screen(Canvas *cv, uint32_t *data, int w, int h) {
    for (int dy = 0; dy < (int)cv->height; dy++) {
        for (int dx = 0; dx < (int)cv->width; dx++) {
            int src_x = (int)((int64_t)dx * w / (int64_t)cv->width);
            int src_y = (int)((int64_t)dy * h / (int64_t)cv->height);
            put_pixel(cv, dx, dy, RGB(data[src_y * w + src_x]));
        }
    }
}


void draw_string_panic(Canvas *cv) {
    int x = 40; 
    int y = 40;
    draw_string(cv, "888     888                  888 888          .d88888b.   .d8888b. ", x, y, rgba(255, 220, 50, 255), 1); y += 16;
    draw_string(cv, "888     888                  888 888         d88P\"Y88b d88P  Y88b ", x, y, rgba(255, 220, 50, 255), 1); y += 16;
    draw_string(cv, "Y88b   d88P 8888b.  888  888 888 888888      888     888  \"Y888b.   ", x, y, rgba(255, 220, 50, 255), 1); y += 16;
    draw_string(cv, "Y88b d88P     \"88b 888  888 888 888         888     888     \"Y88b. ", x, y, rgba(255, 220, 50, 255), 1); y += 16;
    draw_string(cv, "  Y88o88P  .d888888 888  888 888 888  888888 888     888       \"888 ", x, y, rgba(255, 220, 50, 255), 1); y += 16;
    draw_string(cv, "   Y888P   888  888 Y88b 888 888 Y88b.       Y88b. .d88P Y88b  d88P ", x, y, rgba(255, 220, 50, 255), 1); y += 16;
    draw_string(cv, "    Y8P    \"Y888888  \"Y88888 888  \"Y888       \"Y88888P\"   \"Y8888P\"  ", x, y, rgba(255, 220, 50, 255), 1); y += 16;
}

void kernel_panic_init(Canvas *cv, interrupt_frame_t *frame) {
    draw_error_screen(cv, wolf_data, WOLF_W, WOLF_H);
    draw_string_panic(cv);

    int x = 40;              
    int y = 40 + 8 * 16 + 8;

    draw_rectangle(cv, x, y, 8 * 12 * 2, 16 * 2, rgba(255, 0, 0, 255));
    draw_string(cv, "KERNEL PANIC", x, y, rgba(255, 255, 255, 255), 2);
    y += 16 * 2 + 8;

    draw_string(cv, "You broke the system. Congrats.", x, y, rgba(255, 255, 255, 255), 1);
    y += 16 + 4;

    const char *msg = "Unknown exception";
    if      (frame->num == 0)  msg = "Division by zero";
    else if (frame->num == 1)  msg = "Debug";
    else if (frame->num == 2)  msg = "NMI";
    else if (frame->num == 3)  msg = "Breakpoint";
    else if (frame->num == 4)  msg = "Overflow";
    else if (frame->num == 5)  msg = "Bound range exceeded";
    else if (frame->num == 6)  msg = "Invalid operation";
    else if (frame->num == 7)  msg = "Device not available";
    else if (frame->num == 8)  msg = "Double fault";
    else if (frame->num == 10) msg = "Invalid TSS";
    else if (frame->num == 11) msg = "Segment not present";
    else if (frame->num == 12) msg = "Stack segment fault";
    else if (frame->num == 13) msg = "General protection fault";
    else if (frame->num == 14) msg = "Page fault";
    else if (frame->num == 16) msg = "x87 FPU exception";
    else if (frame->num == 17) msg = "Alignment check";
    else if (frame->num == 18) msg = "Machine check";
    else if (frame->num == 19) msg = "SIMD FP exception";

    int rx = cv->width / 2 - 20;
    int ry = 30;
    int rw = cv->width / 2 + 10;  
    int rh = 376;

    draw_rectangle(cv, rx,           ry,          rw, 1,  rgba(80, 255, 200, 255));
    draw_rectangle(cv, rx,           ry + rh - 1, rw, 1,  rgba(80, 255, 200, 255));
    draw_rectangle(cv, rx,           ry,          1,  rh, rgba(80, 255, 200, 255));
    draw_rectangle(cv, rx + rw - 1,  ry,          1,  rh, rgba(80, 255, 200, 255));

    draw_rectangle(cv, rx + 8, ry - 4, 8 * 16 * 2, 9, rgba(10, 10, 30, 255));
    draw_string(cv, " Exception info ", rx + 8, ry - 4, rgba(80, 255, 200, 255), 1);

    int tx = rx + 16;
    int ty = ry + 16;

    draw_string(cv, "Exception :", tx, ty, rgba(150, 150, 150, 255), 2);
    draw_dec(cv, tx + 12 * 8 * 2, ty, frame->num, rgba(255, 80, 80, 255));
    ty += 28;

    draw_string(cv, "Error     :", tx, ty, rgba(150, 150, 150, 255), 2);
    draw_string(cv, msg, tx + 12 * 8 * 2, ty, rgba(255, 80, 80, 255), 2);
    ty += 28;

    draw_string(cv, "Err code  :", tx, ty, rgba(150, 150, 150, 255), 2);
    draw_hex(cv, tx + 12 * 8 * 2, ty, frame->error_code, rgba(255, 220, 50, 255));
    ty += 28 + 8;

    draw_rectangle(cv, tx, ty, rw - 32, 1, rgba(80, 255, 200, 150));
    ty += 12;

    draw_string(cv, "RIP    :", tx, ty, rgba(150, 150, 150, 255), 2);
    draw_hex(cv, tx + 9 * 8 * 2, ty, frame->rip, rgba(255, 220, 50, 255));
    ty += 24;

    draw_string(cv, "RSP    :", tx, ty, rgba(150, 150, 150, 255), 2);
    draw_hex(cv, tx + 9 * 8 * 2, ty, frame->rsp, rgba(255, 220, 50, 255));
    ty += 24;

    draw_string(cv, "RFLAGS :", tx, ty, rgba(150, 150, 150, 255), 2);
    draw_hex(cv, tx + 9 * 8 * 2, ty, frame->rflags, rgba(255, 220, 50, 255));
    ty += 24;

    draw_string(cv, "CS     :", tx, ty, rgba(150, 150, 150, 255), 2);
    draw_hex(cv, tx + 9 * 8 * 2, ty, frame->cs, rgba(255, 220, 50, 255));
    ty += 24;

    draw_string(cv, "RAX    :", tx, ty, rgba(150, 150, 150, 255), 2);
    draw_hex(cv, tx + 9 * 8 * 2, ty, frame->rax, rgba(255, 220, 50, 255));
    ty += 24;

    draw_string(cv, "RBX    :", tx, ty, rgba(150, 150, 150, 255), 2);
    draw_hex(cv, tx + 9 * 8 * 2, ty, frame->rbx, rgba(255, 220, 50, 255));
    ty += 24;

    draw_string(cv, "RCX    :", tx, ty, rgba(150, 150, 150, 255), 2);
    draw_hex(cv, tx + 9 * 8 * 2, ty, frame->rcx, rgba(255, 220, 50, 255));
    ty += 24;

    draw_string(cv, "RDX    :", tx, ty, rgba(150, 150, 150, 255), 2);
    draw_hex(cv, tx + 9 * 8 * 2, ty, frame->rdx, rgba(255, 220, 50, 255));
    ty += 24;

    draw_string(cv, "RSI    :", tx, ty, rgba(150, 150, 150, 255), 2);
    draw_hex(cv, tx + 9 * 8 * 2, ty, frame->rsi, rgba(255, 220, 50, 255));
    ty += 24;

    draw_string(cv, "RDI    :", tx, ty, rgba(150, 150, 150, 255), 2);
    draw_hex(cv, tx + 9 * 8 * 2, ty, frame->rdi, rgba(255, 220, 50, 255));
}