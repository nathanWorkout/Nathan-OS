#include "idt.h"

idt_entry_t idt[256];
idtr_t idtr;

/*
 * num : interrupt vector number (0-255)
 * base : adress of the interrupt handler
 * selector : code segment selector (CS)
 * flags : type and attributes byte
 */
void idt_set_entry(uint8_t num, uint64_t base, uint16_t selector, uint8_t flags) {
    idt[num].offset_low  = base & 0xFFFF;              // first 16 bits
    idt[num].offset_mid  = (base >> 16) & 0xFFFF;      // bits 16 to 31
    idt[num].offset_high = (base >> 32) & 0xFFFFFFFF;  // last 32 bits, with low and mid offset they form the full address
    idt[num].selector    = selector;                   // segment selector (CS normally)
    idt[num].ist         = 0;                          // always set to 0 but must be there for the intel convention
    idt[num].type_attr   = flags;                      // who can trigger the interruption, which door is active...
    idt[num].zero        = 0;                          // always set to 0 but must be there for the intel convention
}

// setup IDTR
void idt_init() {
    idtr.limit = sizeof(idt_entry_t) * 256 - 1; // size of idt, -1 for convention intel
    idtr.base  = (uint64_t)&idt; // memory address to start idt[]

    asm volatile("lidt %0" : : "m"(idtr));
}
