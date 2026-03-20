#include "idt.h"

idt_entry_t idt[256];
idtr_t idtr;

void idt_set_entry(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags) {
    idt[num].offset_low  = base & 0xFFFF;
    idt[num].offset_high = (base >> 16) & 0xFFFF;
    idt[num].selector    = selector;
    idt[num].reserved    = 0;
    idt[num].type_attr   = flags;
}

void idt_init() {
    idtr.limit = sizeof(idt_entry_t) * 256 - 1;
    idtr.base  = (uint32_t)&idt;

    asm volatile("lidt %0" : : "m"(idtr));
}
