#include "isr.h"
#include "idt.h"

void isr_init() {

    void (*isr_table[32])() = {
        isr0,  isr1,  isr2,  isr3,  isr4,  isr5,  isr6,  isr7,
        isr8,  isr9,  isr10, isr11, isr12, isr13, isr14, isr15,
        isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
        isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
    };

    for (int i = 0; i < 32; i++) {
        idt_set_entry(i, (uint32_t)isr_table[i], 0x08, 0x8E);
    }
}

void isr_handler(uint32_t num, uint32_t error_code) {
    volatile unsigned short *vga = (unsigned short *) 0xB8000;
    vga[0] = 0x4F00 | 'E'; 
    vga[1] = 0x4F00 | ('0' + num);  
}


