#ifndef IDT_H
#define IDT_H

#include <stdint.h>

typedef struct __attribute__((packed)) {  // Pas ajouter de padding sinon ca dépasserai la taille
    uint16_t offset_low; // 16 premiers bits
    uint16_t selector;   // sélecteur de segment (cs normalement)
    uint8_t  reserved;   // Toujours mis a 0 mais doit être la pour la convenssion intel
    uint8_t  type_attr;  // Qui peut déclencher l'interruption, quelle poorte est active...
    uint16_t offset_high; // 16 derniers bits, avec offset low ils forme l'adresse complète
} idt_entry_t;

typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint32_t base;
} idtr_t;

extern idt_entry_t idt[256]; // _t est une convenssion qui évite d'écrire struct a chaque fois

#endif
