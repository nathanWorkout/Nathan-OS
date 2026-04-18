#include <stdint.h>
#include "com1.h"

void irq0_handler() {
  
// Test
    static uint32_t ticks = 0;
    ticks++;
//    serial_println("ticks");

}
