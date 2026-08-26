#ifndef TTY_H
#define TTY_H

#include "gfx.h"
#include <stdint.h>

void tty_init(Canvas c);
void tty_clear(void);
void tty_reboot(void);
void tty_set_color(uint32_t c);
void tty_draw_cursor(int visible);
int  tty_get_cols(void);
int  tty_get_rows(void);

void putchar(char c);
void puts(const char *s);
int  printk(const char *fmt, ...);

#endif
