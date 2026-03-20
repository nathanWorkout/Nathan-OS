#include "idt.h"
#include "isr.h"

#include <stdint.h>

void kmain(void) {

    idt_init();
    isr_init();

    volatile int a = 10;
    volatile int b = 0;
    volatile int c = a / b;

    volatile unsigned short *vga = (unsigned short *)0xB8000;
    vga[0] = 0x2F00 | 'H';  
    vga[1] = 0x2F00 | 'E';
    vga[2] = 0x2F00 | 'L';
    vga[3] = 0x2F00 | 'L';
    vga[4] = 0x2F00 | 'O';  
    vga[5] = 0x2F00 | '_';
    vga[6] = 0x2F00 | 'W';
    vga[7] = 0x2F00 | 'O';
    vga[8] = 0x2F00 | 'R';
    vga[9] = 0x2F00 | 'L';
    vga[10] = 0x2F00 | 'D';
    while (1); 
}
