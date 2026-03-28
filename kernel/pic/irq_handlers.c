#include "com1.h"
#include <stdint.h>

void irq0_handler() {
// Test
    static uint32_t ticks = 0;
    ticks++;
    serial_print("tick\n");
}
