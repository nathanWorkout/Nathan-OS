#include "pic8089.h"
#include "io.h"
#include <stdint.h>


void pic_init() {
    // | to combine two flags
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);    // initialize master PIC
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);    // initialize slave PIC
    outb(0xA0, 0x00); // delay so the CPU can process

    // Remapping IRQs
    outb(PIC1_DATA, PIC1_OFFSET);
    outb(PIC2_DATA, PIC2_OFFSET);
    outb(0x80, 0x00);

    // Cascade mode
    outb(PIC1_DATA, ICW3_1);
    outb(PIC2_DATA, ICW3_2);
    outb(0x80, 0x00);

    // 8086 mode
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);
    outb(0x80, 0x00);

    // mask all IRQs
    outb(PIC1_DATA, IRQ);
    outb(PIC2_DATA, IRQ);
}

// Send End Of Interrupt signal to the PIC so it can handle the next interrupt
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
	value = noActive | (1 << irq); // Set bit to 1 -> irq ignored
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
	value = noActive & ~(1 << irq); // Set bit to 0 -> irq active
	outb(PIC1_DATA, value);
    } else {
	uint8_t active = inb(PIC2_DATA);
	value = active & ~(1 << (irq - 8));
	outb(PIC2_DATA, value);
    }
}
