#include "pic8089.h"
#include "io.h"
#include <stdint.h>


void pic_init() {
    // | pour combiner 2 flags
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4); // Initialisation maitre (master)
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);    // Initilaisation esclave (slave)
    outb(0x80, 0x00); // Delai pour que le cpu puisse traiter

    // Remapping
    outb(PIC1_DATA, PIC1_OFFSET);
    outb(PIC2_DATA, PIC2_OFFSET);
    outb(0x80, 0x00);

    // Cascade
    outb(PIC1_DATA, ICW3_1);
    outb(PIC2_DATA, ICW3_2);
    outb(0x80, 0x00);

    // Mode 8086
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);
    outb(0x80, 0x00);

    // Masquer les irq
    outb(PIC1_DATA, IRQ); 
    outb(PIC2_DATA, IRQ);
}

// Une fois qu'une interruptio a eu lieu il peut en traiter d'autres
void pic_send_eoi(uint8_t irq) {
    if(irq >= 8) {
	outb(PIC2_COMMAND, PIC_EOI); // Slave
	outb(PIC1_COMMAND, PIC_EOI); // Master
    } else {
	outb(PIC1_COMMAND, PIC_EOI);
    }
}

void pic_set_mask(uint8_t irq) {
    uint8_t value;

    if(irq < 8) {
	uint8_t noActive = inb(PIC1_DATA);
	value = noActive | (1 << irq); // Pour mettre a 1 -> irq ignoré
	outb(PIC1_DATA, value);
    } else if(irq >= 8) {
        uint8_t active = inb(PIC2_DATA);
	value = active | (1 << (irq - 8));
	outb(PIC2_DATA, value);
    }
}

void pic_clear_mask(uint8_t irq) {
    uint8_t value;

    if(irq < 8) {
	uint8_t noActive = inb(PIC1_DATA);
	value = noActive & ~(1 << irq); // Mettre a 0 -> irq active
	outb(PIC1_DATA, value);
    } else if (irq >= 8) {
	uint8_t active = inb(PIC2_DATA);
	value = active & ~(1 << (irq - 8));
	outb(PIC2_DATA, value);
    }
}
