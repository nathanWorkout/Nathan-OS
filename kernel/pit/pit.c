#include "io.h"
#include "pit.h"
#include <stdint.h>

// Canal 0
// Mode 3 (square wave)
// Access 16 bits
// Binary format
void pit_init(uint32_t freq) {
    int divider = 1193182 / freq;
    outb(COMMAND_REGISTER, 0x36);
    outb(CANAL0, divider & 0xFF); // low byte
    outb(CANAL0, divider >> 8);   // high byte
}
