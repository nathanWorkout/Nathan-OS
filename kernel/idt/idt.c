#include "idt.h"
#include "com1.h"

idt_entry_t idt[256];
idtr_t idtr;

void idt_set_entry(uint8_t num, uint64_t base, uint16_t selector, uint8_t flags) {
    idt[num].offset_low  = base & 0xFFFF;              // 16 premiers bits
    idt[num].offset_mid  = (base >> 16) & 0xFFFF;      // bits 16 à 31
    idt[num].offset_high = (base >> 32) & 0xFFFFFFFF;  // 32 derniers bits, avec offset low et mid ils forme l'adresse complète
    idt[num].selector    = selector;                   // sélecteur de segment (cs normalement)
    idt[num].ist         = 0;                          // Toujours mis a 0 mais doit être la pour la convenssion intel
    idt[num].type_attr   = flags;                      // Qui peut déclencher l'interruption, quelle poorte est active...
    idt[num].zero        = 0;                          // Toujours mis a 0 mais doit être la pour la convenssion intel
}

void idt_init() {
    idtr.limit = sizeof(idt_entry_t) * 256 - 1;
    idtr.base  = (uint64_t)&idt;

    asm volatile("lidt %0" : : "m"(idtr));
}
