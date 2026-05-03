#include <stdint.h>
#include "com1.h"
#include "keyboard.h"

volatile uint64_t pit_ticks = 0;

void irq0_handler() {
    pit_ticks++;
}
