#ifndef PIC8089_F
#define PIC8089_F
#include <stdint.h>

// Cascade : CPU <- PIC Maitre (IRQ 0-7)
//                     ^
//                     |
//                  PIC esclave (IRQ 8-15 branché sur IRQ2 du maitre)

// Port I/O
// C'est l'adresse physique pour communiquer avec le PIC
#define PIC1_COMMAND 0x20 // PIC maitre
#define PIC1_DATA    0x21 // Port data du master (master est celui connecter au cpu)
#define PIC2_COMMAND 0xA0 // Envoie des ordres (slave) slave = connecter au master
#define PIC2_DATA    0xA1 // Envoie des données (slave)

// ICW1
// Séquence d'initialisation
#define ICW1_INIT    0x10 // Démarre l'init
#define ICW1_ICW4    0x01 // Envoie ICW4 après
			  
// Cascade
#define ICW3_1       0x04
#define ICW3_2       0x02

// ICW4
#define ICW4_8086    0x01 // On est en mode 8086

// EOI
#define PIC_EOI      0x20 // Dire que c fini au PIC

// Offset de remapping pour pas qu'il y ai de conflit avec le CPU
// ICW2
#define PIC1_OFFSET  0x20
#define PIC2_OFFSET  0x28

// Masquer les irq
#define IRQ          0xFF

// Foncitons
void pic_init();
void pic_send_eoi(uint8_t irq);
void pic_set_mask(uint8_t irq);
void pic_clear_mask(uint8_t irq);

#endif
