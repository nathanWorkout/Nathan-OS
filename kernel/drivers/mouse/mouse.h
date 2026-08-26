#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

#define PS2_DATA    0x60
#define PS2_STATUS  0x64
#define PS2_CMD     0x64

typedef struct {
    int32_t x;
    int32_t y;
    int32_t x_min;
    int32_t y_min;
    int32_t x_max;
    int32_t y_max;
    uint8_t left;
    uint8_t right;
    uint8_t middle;
} MouseState;

void mouse_init(void);
void mouse_irq_handler(void);
MouseState *mouse_get_state(void);

#endif