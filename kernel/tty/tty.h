#ifndef TTY_H
#define TTY_H

#include <stdint.h>

void enable_cursor(uint8_t cursor_start, uint8_t cursor_end);
void update_cursor(int x, int y);
uint16_t get_cursor_position(void);
void tty_init();
void putchar(char c);
void puts(char *s);
int printk(const char *fmt, ...);

#endif
