#ifndef KERNEL_PANIC_H
#define KERNEL_PANIC_H

#include <stdint.h>
#include "gfx.h"
#include "isr.h"

void draw_error_screen(Canvas *cv, uint32_t *data, int w, int h);
void draw_string_panic(Canvas *cv);
void kernel_panic_init(Canvas *cv, interrupt_frame_t *frame);

#endif