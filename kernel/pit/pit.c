#include "io.h"
#include "pit.h"
#include <stdint.h>

// Canal 0
// Mode 3 (square wave)
// Accès 16 bits 
// Format binaire
void pit_init(uint32_t freq) {
    int divider = 1193182 / freq;
    outb(COMMAND_REGISTER, 0x36); 
    outb(CANAL0, divider & 0xFF); // Octet bas
    outb(CANAL0, divider >> 8);   // Octet haut
}
