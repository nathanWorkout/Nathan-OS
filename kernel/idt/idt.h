#ifndef IDT_H
#define IDT_H

#include <stdint.h>

typedef struct __attribute__((packed)) {
    uint16_t offset_low;     // bits 0-15
    uint16_t selector;       // segment selector (CS)
    uint8_t  ist;            // bits 0-2 = IST, le reste à 0
    uint8_t  type_attr;      // type + flags
    uint16_t offset_mid;     // bits 16-31
    uint32_t offset_high;    // bits 32-63
    uint32_t zero;           // réservé (doit être 0 par intel)
} idt_entry_t;

typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint64_t base;           
} idtr_t;

extern idt_entry_t idt[256];

void idt_set_entry(uint8_t num, uint64_t base, uint16_t selector, uint8_t flags);
void idt_init();

#endif
