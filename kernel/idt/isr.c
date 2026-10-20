#include "isr.h"
#include "idt.h"
#include "kernel_panic.h"
#include "framebuffer.h"
#include "gfx.h"
#include <stdint.h>

// Registers the assembly stubs in the IDT and defines the C handler called by isr_common during a CPU exception

extern void irq0(); // PIT timer
extern void irq1(); // keyboard
extern void irq12(); // PS/2 mouse

/*
 * kernel code segment : segment used to execute interrupt handlers
 * idt gate interrupt  : 64 bit interrupt gate (disables interrupts on entry, ring 0 only)
 */
#define KERNEL_CODE_SEGMENT 0x08 // kernel code, GDT, ring 0
#define IDT_GATE_INTERRUPT 0x8E // present, ring 0, not a segment descriptor, 64 bits interrupt gate

void isr_init() {
    static void (*isr_table[32])() = {
        isr0,  isr1,  isr2,  isr3,  isr4,  isr5,  isr6,  isr7,
        isr8,  isr9,  isr10, isr11, isr12, isr13, isr14, isr15,
        isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
        isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
    };

    for (int i = 0; i < 32; i++) {
        idt_set_entry(i, (uint64_t)isr_table[i], KERNEL_CODE_SEGMENT, IDT_GATE_INTERRUPT);
    }
    idt_set_entry(0x2C, (uint64_t)irq12, KERNEL_CODE_SEGMENT, IDT_GATE_INTERRUPT);
    idt_set_entry(32, (uint64_t)irq0, KERNEL_CODE_SEGMENT, IDT_GATE_INTERRUPT);
    idt_set_entry(33, (uint64_t)irq1, KERNEL_CODE_SEGMENT, IDT_GATE_INTERRUPT); // IST disable
}

// called by isr_commun (assembbly) on any COPU exeption
void isr_handler(interrupt_frame_t* frame) {
    Canvas cv = fb_get_canvas();
    kernel_panic_init(&cv, frame);
}
