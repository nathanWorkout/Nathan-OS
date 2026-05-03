#include "isr.h"
#include "idt.h"
#include "com1.h"
#include "kernel_panic.h"
#include "framebuffer.h"
#include "gfx.h"
#include <stdint.h>
#include "gdt.h"

extern void irq0();
extern void irq1();

void isr_init() {
    void (*isr_table[32])() = {
        isr0,  isr1,  isr2,  isr3,  isr4,  isr5,  isr6,  isr7,
        isr8,  isr9,  isr10, isr11, isr12, isr13, isr14, isr15,
        isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
        isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
    };

    for (int i = 0; i < 32; i++)
        idt_set_entry(i, (uint64_t)isr_table[i], 0x08, 0x8e);

    idt_set_entry(32, (uint64_t)irq0, 0x08, 0x8e);
    idt_set_entry(33, (uint64_t)irq1, 0x08, 0x8e); // IST désactivé
}

void isr_handler(interrupt_frame_t* frame) {
    Canvas cv = fb_get_canvas();
    kernel_panic_init(&cv, frame);
    while(1);
}