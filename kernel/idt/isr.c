#include "isr.h"
#include "idt.h"

void isr_init() {
    void (*isr_table[32])() = {
        isr0,  isr1,  isr2,  isr3,  isr4,  isr5,  isr6,  isr7,
        isr8,  isr9,  isr10, isr11, isr12, isr13, isr14, isr15,
        isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
        isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
    };
    for (int i = 0; i < 32; i++) {
        idt_set_entry(i, (uint32_t)isr_table[i], 0x08, 0x8e);
    }
}

static void vga_puts(volatile unsigned short *vga, int ligne, int col, const char *s, unsigned short couleur) {
    while (*s) {
        vga[ligne * 80 + col] = couleur | (unsigned char)*s;
        col++;
        s++;
    }
}


static void vga_puthex(volatile unsigned short *vga, int ligne, int col, uint32_t val, unsigned short couleur) {
    char buf[11];
    buf[0] = '0';
    buf[1] = 'x';
    const char hex[] = "0123456789ABCDEF";
    for (int i = 7; i >= 0; i--) {
        buf[2 + i] = hex[val & 0xF];
        val >>= 4;
    }
    buf[10] = '\0';
    vga_puts(vga, ligne, col, buf, couleur);
}

static void vga_putdec(volatile unsigned short *vga, int ligne, int col, uint32_t val, unsigned short couleur) {
    char buf[12];
    int i = 10;
    buf[11] = '\0';
    if (val == 0) {
        buf[i--] = '0';
    } else {
        while (val > 0) {
            buf[i--] = '0' + (val % 10);
            val /= 10;
        }
    }
    vga_puts(vga, ligne, col, buf + i + 1, couleur);
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "nd"(port));
}

void isr_handler(uint32_t num, uint32_t error_code, uint32_t eip) {

    outb(0x3d4, 0x0a);
    outb(0x3d5, 0x20);

    volatile unsigned short *vga = (unsigned short *) 0xb8000;


    for(int i = 0; i < 80 * 25; i++)
        vga[i] = 0x1f00 | ' ';

    // Lune
    vga[8 * 80 + 20] = 0xEF00 | '.';
    vga[8 * 80 + 21] = 0xEF00 | '.';
    vga[8 * 80 + 22] = 0xEF00 | '-';
    vga[8 * 80 + 23] = 0xEF00 | '-';
    vga[8 * 80 + 24] = 0xEF00 | '-';
    vga[8 * 80 + 25] = 0xEF00 | '-';
    vga[8 * 80 + 26] = 0xEF00 | '-';
    vga[8 * 80 + 27] = 0xEF00 | '-';
    vga[8 * 80 + 28] = 0xEF00 | '.';
    vga[8 * 80 + 29] = 0xEF00 | '.';

    vga[9 * 80 + 18] = 0xEF00 | '-';
    vga[9 * 80 + 19] = 0xEF00 | '/';
    vga[9 * 80 + 20] = 0xEE00 | ' ';
    vga[9 * 80 + 21] = 0xEE00 | ' ';
    vga[9 * 80 + 22] = 0xEE00 | ' ';
    vga[9 * 80 + 23] = 0xEE00 | ' ';
    vga[9 * 80 + 24] = 0xEE00 | ' ';
    vga[9 * 80 + 25] = 0xEE00 | ' ';
    vga[9 * 80 + 26] = 0xEE00 | ' ';
    vga[9 * 80 + 27] = 0xEE00 | ' ';
    vga[9 * 80 + 28] = 0xEE00 | ' ';
    vga[9 * 80 + 29] = 0xEE00 | ' ';
    vga[9 * 80 + 30] = 0xEF00 | '\\';
    vga[9 * 80 + 31] = 0xEF00 | '-';

    vga[10 * 80 + 16] = 0xEF00 | '/';
    vga[10 * 80 + 17] = 0xEE00 | ' ';
    vga[10 * 80 + 18] = 0xEE00 | ' ';
    vga[10 * 80 + 19] = 0xEE00 | ' ';
    vga[10 * 80 + 20] = 0xEE00 | ' ';
    vga[10 * 80 + 21] = 0xEE00 | ' ';
    vga[10 * 80 + 22] = 0xEE00 | ' ';
    vga[10 * 80 + 23] = 0xEE00 | ' ';
    vga[10 * 80 + 24] = 0xEE00 | ' ';
    vga[10 * 80 + 25] = 0xEE00 | ' ';
    vga[10 * 80 + 26] = 0xEE00 | ' ';
    vga[10 * 80 + 27] = 0xEE00 | ' ';
    vga[10 * 80 + 28] = 0xEE00 | ' ';
    vga[10 * 80 + 29] = 0xEE00 | ' ';
    vga[10 * 80 + 30] = 0xEE00 | ' ';
    vga[10 * 80 + 31] = 0xEE00 | ' ';
    vga[10 * 80 + 32] = 0xEE00 | ' ';
    vga[10 * 80 + 33] = 0xEF00 | '\\';

    vga[11 * 80 + 15] = 0xEF00 | '/';
    vga[11 * 80 + 16] = 0xEE00 | ' ';
    vga[11 * 80 + 17] = 0xEE00 | ' ';
    vga[11 * 80 + 18] = 0xEE00 | ' ';
    vga[11 * 80 + 19] = 0xEE00 | ' ';
    vga[11 * 80 + 20] = 0xEE00 | ' ';
    vga[11 * 80 + 21] = 0xEE00 | ' ';
    vga[11 * 80 + 22] = 0xEE00 | ' ';
    vga[11 * 80 + 23] = 0xEE00 | ' ';
    vga[11 * 80 + 24] = 0xEE00 | ' ';
    vga[11 * 80 + 25] = 0xEE00 | ' ';
    vga[11 * 80 + 26] = 0xEE00 | ' ';
    vga[11 * 80 + 27] = 0xEE00 | ' ';
    vga[11 * 80 + 28] = 0xEE00 | ' ';
    vga[11 * 80 + 29] = 0xEE00 | ' ';
    vga[11 * 80 + 30] = 0xEE00 | ' ';
    vga[11 * 80 + 31] = 0xEE00 | ' ';
    vga[11 * 80 + 32] = 0xEE00 | ' ';
    vga[11 * 80 + 33] = 0xEE00 | ' ';
    vga[11 * 80 + 34] = 0xEf00 | '\\';

    vga[12 * 80 + 14] = 0xEf00 | '|';
    vga[12 * 80 + 15] = 0xEE00 | ' ';
    vga[12 * 80 + 16] = 0xEE00 | ' ';
    vga[12 * 80 + 17] = 0xEE00 | ' ';
    vga[12 * 80 + 18] = 0xEE00 | ' ';
    vga[12 * 80 + 19] = 0xEE00 | ' ';
    vga[12 * 80 + 20] = 0xEE00 | ' ';
    vga[12 * 80 + 21] = 0xEE00 | ' ';
    vga[12 * 80 + 22] = 0xEE00 | ' ';
    vga[12 * 80 + 23] = 0xEE00 | ' ';
    vga[12 * 80 + 24] = 0xEE00 | ' ';
    vga[12 * 80 + 25] = 0xEE00 | ' ';
    vga[12 * 80 + 26] = 0xEE00 | ' ';
    vga[12 * 80 + 27] = 0xEE00 | ' ';
    vga[12 * 80 + 28] = 0xEE00 | ' ';
    vga[12 * 80 + 29] = 0xEE00 | ' ';
    vga[12 * 80 + 30] = 0xEE00 | ' ';
    vga[12 * 80 + 31] = 0xEE00 | ' ';
    vga[12 * 80 + 32] = 0xEE00 | ' ';
    vga[12 * 80 + 33] = 0xEE00 | ' ';
    vga[12 * 80 + 34] = 0xEE00 | ' ';
    vga[12 * 80 + 35] = 0xEf00 | '|';

    vga[13 * 80 + 14] = 0xEf00 | '|';
    vga[13 * 80 + 15] = 0xEE00 | ' ';
    vga[13 * 80 + 16] = 0xEE00 | ' ';
    vga[13 * 80 + 17] = 0xEE00 | ' ';
    vga[13 * 80 + 18] = 0xEE00 | ' ';
    vga[13 * 80 + 19] = 0xEE00 | ' ';
    vga[13 * 80 + 20] = 0xEE00 | ' ';
    vga[13 * 80 + 21] = 0xEE00 | ' ';
    vga[13 * 80 + 22] = 0xEE00 | ' ';
    vga[13 * 80 + 23] = 0xEE00 | ' ';
    vga[13 * 80 + 24] = 0xEE00 | ' ';
    vga[13 * 80 + 25] = 0xEE00 | ' ';
    vga[13 * 80 + 26] = 0xEE00 | ' ';
    vga[13 * 80 + 27] = 0xEE00 | ' ';
    vga[13 * 80 + 28] = 0xEE00 | ' ';
    vga[13 * 80 + 29] = 0xEE00 | ' ';
    vga[13 * 80 + 30] = 0xEE00 | ' ';
    vga[13 * 80 + 31] = 0xEE00 | ' ';
    vga[13 * 80 + 32] = 0xEE00 | ' ';
    vga[13 * 80 + 33] = 0xEE00 | ' ';
    vga[13 * 80 + 34] = 0xEE00 | ' ';
    vga[13 * 80 + 35] = 0xEf00 | '|';

    vga[14 * 80 + 14] = 0xEf00 | '|';
    vga[14 * 80 + 15] = 0xEE00 | ' ';
    vga[14 * 80 + 16] = 0xEE00 | ' ';
    vga[14 * 80 + 17] = 0xEE00 | ' ';
    vga[14 * 80 + 18] = 0xEE00 | ' ';
    vga[14 * 80 + 19] = 0xEE00 | ' ';
    vga[14 * 80 + 20] = 0xEE00 | ' ';
    vga[14 * 80 + 21] = 0xEE00 | ' ';
    vga[14 * 80 + 22] = 0xEE00 | ' ';
    vga[14 * 80 + 23] = 0xEE00 | ' ';
    vga[14 * 80 + 24] = 0xEE00 | ' ';
    vga[14 * 80 + 25] = 0xEE00 | ' ';
    vga[14 * 80 + 26] = 0xEE00 | ' ';
    vga[14 * 80 + 27] = 0xEE00 | ' ';
    vga[14 * 80 + 28] = 0xEE00 | ' ';
    vga[14 * 80 + 29] = 0xEE00 | ' ';
    vga[14 * 80 + 30] = 0xEE00 | ' ';
    vga[14 * 80 + 31] = 0xEE00 | ' ';
    vga[14 * 80 + 32] = 0xEE00 | ' ';
    vga[14 * 80 + 33] = 0xEE00 | ' ';
    vga[14 * 80 + 34] = 0xEE00 | ' ';
    vga[14 * 80 + 35] = 0xEf00 | '|';

    vga[15 * 80 + 15] = 0xEf00 | '\\';
    vga[15 * 80 + 16] = 0xEE00 | ' ';
    vga[15 * 80 + 17] = 0xEE00 | ' ';
    vga[15 * 80 + 18] = 0xEE00 | ' ';
    vga[15 * 80 + 19] = 0xEE00 | ' ';
    vga[15 * 80 + 20] = 0xEE00 | ' ';
    vga[15 * 80 + 21] = 0xEE00 | ' ';
    vga[15 * 80 + 22] = 0xEE00 | ' ';
    vga[15 * 80 + 23] = 0xEE00 | ' ';
    vga[15 * 80 + 24] = 0xEE00 | ' ';
    vga[15 * 80 + 25] = 0xEE00 | ' ';
    vga[15 * 80 + 26] = 0xEE00 | ' ';
    vga[15 * 80 + 27] = 0xEE00 | ' ';
    vga[15 * 80 + 28] = 0xEE00 | ' ';
    vga[15 * 80 + 29] = 0xEE00 | ' ';
    vga[15 * 80 + 30] = 0xEE00 | ' ';
    vga[15 * 80 + 31] = 0xEE00 | ' ';
    vga[15 * 80 + 32] = 0xEE00 | ' ';
    vga[15 * 80 + 33] = 0xEE00 | ' ';
    vga[15 * 80 + 34] = 0xEf00 | '/';

    vga[16 * 80 + 16] = 0xEf00 | '\\';
    vga[16 * 80 + 17] = 0xEE00 | ' ';
    vga[16 * 80 + 18] = 0xEE00 | ' ';
    vga[16 * 80 + 19] = 0xEE00 | ' ';
    vga[16 * 80 + 20] = 0xEE00 | ' ';
    vga[16 * 80 + 21] = 0xEE00 | ' ';
    vga[16 * 80 + 22] = 0xEE00 | ' ';
    vga[16 * 80 + 23] = 0xEE00 | ' ';
    vga[16 * 80 + 24] = 0xEE00 | ' ';
    vga[16 * 80 + 25] = 0xEE00 | ' ';
    vga[16 * 80 + 26] = 0xEE00 | ' ';
    vga[16 * 80 + 27] = 0xEE00 | ' ';
    vga[16 * 80 + 28] = 0xEE00 | ' ';
    vga[16 * 80 + 29] = 0xEE00 | ' ';
    vga[16 * 80 + 30] = 0xEE00 | ' ';
    vga[16 * 80 + 31] = 0xEE00 | ' ';
    vga[16 * 80 + 32] = 0xEE00 | ' ';
    vga[16 * 80 + 33] = 0xEf00 | '/';

    vga[17 * 80 + 18] = 0xEf00 | '-';
    vga[17 * 80 + 19] = 0xEf00 | '\\';
    vga[17 * 80 + 20] = 0xEE00 | ' ';
    vga[17 * 80 + 21] = 0xEE00 | ' ';
    vga[17 * 80 + 22] = 0xEE00 | ' ';
    vga[17 * 80 + 23] = 0xEE00 | ' ';
    vga[17 * 80 + 24] = 0xEE00 | ' ';
    vga[17 * 80 + 25] = 0xEE00 | ' ';
    vga[17 * 80 + 26] = 0xEE00 | ' ';
    vga[17 * 80 + 27] = 0xEE00 | ' ';
    vga[17 * 80 + 28] = 0xEE00 | ' ';
    vga[17 * 80 + 29] = 0xEE00 | ' ';
    vga[17 * 80 + 30] = 0xEf00 | '/';
    vga[17 * 80 + 31] = 0xEf00 | '-';

    vga[18 * 80 + 20] = 0xEF00 | '\'';
    vga[18 * 80 + 21] = 0xEF00 | '.';
    vga[18 * 80 + 22] = 0xEE00 | ' ';
    vga[18 * 80 + 23] = 0xEE00 | ' ';
    vga[18 * 80 + 24] = 0xEE00 | ' ';
    vga[18 * 80 + 25] = 0xEE00 | ' ';
    vga[18 * 80 + 26] = 0xEE00 | ' ';
    vga[18 * 80 + 27] = 0xEE00 | ' ';
    vga[18 * 80 + 28] = 0xEF00 | '\\';
    vga[18 * 80 + 29] = 0xEF00 | '\'';

    // Loup
    vga[10 * 80 + 22] = 0x0f00 | '|';
    vga[10 * 80 + 23] = 0x0f00 | '\\';

    vga[11 * 80 + 21] = 0x0f00 | '|';
    vga[11 * 80 + 22] = 0x0f00 | 'v';
    vga[11 * 80 + 23] = 0x0000 | ' ';
    vga[11 * 80 + 24] = 0x0f00 | '\\';

    vga[12 * 80 + 21] = 0x0f00 | '|';
    vga[12 * 80 + 22] = 0x0000 | ' ';
    vga[12 * 80 + 23] = 0x0f00 | '\'';
    vga[12 * 80 + 24] = 0x0000 | ' ';
    vga[12 * 80 + 25] = 0x0f00 | '\\';

    vga[13 * 80 + 21] = 0x0f00 | ')';
    vga[13 * 80 + 22] = 0x0000 | ' ';
    vga[13 * 80 + 23] = 0x0000 | ' ';
    vga[13 * 80 + 24] = 0x0f00 | ',';
    vga[13 * 80 + 25] = 0x0f00 | '_';
    vga[13 * 80 + 26] = 0x0f00 | '\\';

    vga[14 * 80 + 20] = 0x0f00 | '/';
    vga[14 * 80 + 21] = 0x0000 | ' ';
    vga[14 * 80 + 22] = 0x0000 | ' ';
    vga[14 * 80 + 23] = 0x0000 | ' ';
    vga[14 * 80 + 24] = 0x0f00 | '|';

    vga[15 * 80 + 19] = 0x0f00 | '/';
    vga[15 * 80 + 20] = 0x0000 | ' ';
    vga[15 * 80 + 21] = 0x0000 | ' ';
    vga[15 * 80 + 22] = 0x0000 | ' ';
    vga[15 * 80 + 23] = 0x0000 | ' ';
    vga[15 * 80 + 24] = 0x0000 | ' ';
    vga[15 * 80 + 25] = 0x0f00 | '\\';

    vga[16 * 80 + 19] = 0x0f00 | '|';
    vga[16 * 80 + 20] = 0x0000 | ' ';
    vga[16 * 80 + 21] = 0x0000 | ' ';
    vga[16 * 80 + 22] = 0x0000 | ' ';
    vga[16 * 80 + 23] = 0x0000 | ' ';
    vga[16 * 80 + 24] = 0x0000 | ' ';
    vga[16 * 80 + 25] = 0x0000 | ' ';
    vga[16 * 80 + 26] = 0x0f00 | '\\';

    vga[17 * 80 + 20] = 0x0f00 | '\\';
    vga[17 * 80 + 21] = 0x0000 | ' ';
    vga[17 * 80 + 22] = 0x0000 | ' ';
    vga[17 * 80 + 23] = 0x0000 | ' ';
    vga[17 * 80 + 24] = 0x0000 | ' ';
    vga[17 * 80 + 25] = 0x0000 | ' ';
    vga[17 * 80 + 26] = 0x0000 | ' ';
    vga[17 * 80 + 27] = 0x0f00 | '\\';

    vga[18 * 80 + 21] = 0x0f00 | '|';
    vga[18 * 80 + 22] = 0x0000 | ' ';
    vga[18 * 80 + 23] = 0x0000 | ' ';
    vga[18 * 80 + 24] = 0x0000 | ' ';
    vga[18 * 80 + 25] = 0x0000 | ' ';
    vga[18 * 80 + 26] = 0x0000 | ' ';
    vga[18 * 80 + 27] = 0x0000 | ' ';
    vga[18 * 80 + 28] = 0x0f00 | '\\';


    vga[19 * 80 + 21] = 0x0f00 | '|';
    vga[19 * 80 + 22] = 0x0000 | ' ';
    vga[19 * 80 + 23] = 0x0f00 | '|';
    vga[19 * 80 + 24] = 0x0f00 | '\\';
    vga[19 * 80 + 25] = 0x0000 | ' ';
    vga[19 * 80 + 26] = 0x0000 | ' ';
    vga[19 * 80 + 27] = 0x0000 | ' ';
    vga[19 * 80 + 28] = 0x0000 | ' ';
    vga[19 * 80 + 29] = 0x0f00 | '|';

    
    vga[20 * 80 + 21] = 0x0f00 | '/';
    vga[20 * 80 + 22] = 0x0000 | ' ';
    vga[20 * 80 + 23] = 0x0f00 | '|';
    vga[20 * 80 + 24] = 0x0f00 | '_';
    vga[20 * 80 + 25] = 0x0f00 | '\'';
    vga[20 * 80 + 26] = 0x0f00 | '.';
    vga[20 * 80 + 27] = 0x0000 | ' ';
    vga[20 * 80 + 28] = 0x0000 | ' ';
    vga[20 * 80 + 29] = 0x0f00 | '/';

    // Sol
    for(int i = 21 * 80; i < 80 * 25; i++)
        vga[i] = 0x2f00 | ' ';

    
    for(int c = 0; c < 80; c++) {
        if(c >= 21 && c <= 29)
            vga[20 * 80 + c] = 0x0000 | ' ';
        else
            vga[20 * 80 + c] = 0x1000 | '_';
    }

    vga[20 * 80 + 21] = 0x0f00 | '/';
    vga[20 * 80 + 23] = 0x0f00 | '|';
    vga[20 * 80 + 24] = 0x0f00 | '_';
    vga[20 * 80 + 25] = 0x0f00 | '\'';
    vga[20 * 80 + 26] = 0x0f00 | '.';
    vga[20 * 80 + 29] = 0x0f00 | '/';

    // Text à droite
    // NATHAN-OS
    // N
    vga[1 * 80 + 44] = 0x1f00 | '#';
    vga[1 * 80 + 45] = 0x1f00 | ' ';
    vga[1 * 80 + 46] = 0x1f00 | '#';
    vga[2 * 80 + 44] = 0x1f00 | '#';
    vga[2 * 80 + 45] = 0x1f00 | '*';
    vga[2 * 80 + 46] = 0x1f00 | '#';
    vga[3 * 80 + 44] = 0x1f00 | '#';
    vga[3 * 80 + 45] = 0x1f00 | '@';
    vga[3 * 80 + 46] = 0x1f00 | '#';
    vga[4 * 80 + 44] = 0x1f00 | '#';
    vga[4 * 80 + 45] = 0x1f00 | ' ';
    vga[4 * 80 + 46] = 0x1f00 | '#';
    vga[5 * 80 + 44] = 0x1f00 | '#';
    vga[5 * 80 + 45] = 0x1f00 | ' ';
    vga[5 * 80 + 46] = 0x1f00 | '#';

    // A
    vga[1 * 80 + 48] = 0x1f00 | ' ';
    vga[1 * 80 + 49] = 0x1f00 | '@';
    vga[1 * 80 + 50] = 0x1f00 | ' ';
    vga[2 * 80 + 48] = 0x1f00 | '#';
    vga[2 * 80 + 49] = 0x1f00 | ' ';
    vga[2 * 80 + 50] = 0x1f00 | '#';
    vga[3 * 80 + 48] = 0x1f00 | '#';
    vga[3 * 80 + 49] = 0x1f00 | '*';
    vga[3 * 80 + 50] = 0x1f00 | '#';
    vga[4 * 80 + 48] = 0x1f00 | '#';
    vga[4 * 80 + 49] = 0x1f00 | ' ';
    vga[4 * 80 + 50] = 0x1f00 | '#';
    vga[5 * 80 + 48] = 0x1f00 | '#';
    vga[5 * 80 + 49] = 0x1f00 | ' ';
    vga[5 * 80 + 50] = 0x1f00 | '#';

    // T
    vga[1 * 80 + 52] = 0x1f00 | '@';
    vga[1 * 80 + 53] = 0x1f00 | '#';
    vga[1 * 80 + 54] = 0x1f00 | '@';
    vga[2 * 80 + 52] = 0x1f00 | ' ';
    vga[2 * 80 + 53] = 0x1f00 | '#';
    vga[2 * 80 + 54] = 0x1f00 | ' ';
    vga[3 * 80 + 52] = 0x1f00 | ' ';
    vga[3 * 80 + 53] = 0x1f00 | '#';
    vga[3 * 80 + 54] = 0x1f00 | ' ';
    vga[4 * 80 + 52] = 0x1f00 | ' ';
    vga[4 * 80 + 53] = 0x1f00 | '#';
    vga[4 * 80 + 54] = 0x1f00 | ' ';
    vga[5 * 80 + 52] = 0x1f00 | ' ';
    vga[5 * 80 + 53] = 0x1f00 | '*';
    vga[5 * 80 + 54] = 0x1f00 | ' ';

    // H
    vga[1 * 80 + 56] = 0x1f00 | '#';
    vga[1 * 80 + 57] = 0x1f00 | ' ';
    vga[1 * 80 + 58] = 0x1f00 | '#';
    vga[2 * 80 + 56] = 0x1f00 | '#';
    vga[2 * 80 + 57] = 0x1f00 | ' ';
    vga[2 * 80 + 58] = 0x1f00 | '#';
    vga[3 * 80 + 56] = 0x1f00 | '#';
    vga[3 * 80 + 57] = 0x1f00 | '*';
    vga[3 * 80 + 58] = 0x1f00 | '#';
    vga[4 * 80 + 56] = 0x1f00 | '#';
    vga[4 * 80 + 57] = 0x1f00 | ' ';
    vga[4 * 80 + 58] = 0x1f00 | '#';
    vga[5 * 80 + 56] = 0x1f00 | '#';
    vga[5 * 80 + 57] = 0x1f00 | ' ';
    vga[5 * 80 + 58] = 0x1f00 | '#';

    // A
    vga[1 * 80 + 60] = 0x1f00 | ' ';
    vga[1 * 80 + 61] = 0x1f00 | '@';
    vga[1 * 80 + 62] = 0x1f00 | ' ';
    vga[2 * 80 + 60] = 0x1f00 | '#';
    vga[2 * 80 + 61] = 0x1f00 | ' ';
    vga[2 * 80 + 62] = 0x1f00 | '#';
    vga[3 * 80 + 60] = 0x1f00 | '#';
    vga[3 * 80 + 61] = 0x1f00 | '*';
    vga[3 * 80 + 62] = 0x1f00 | '#';
    vga[4 * 80 + 60] = 0x1f00 | '#';
    vga[4 * 80 + 61] = 0x1f00 | ' ';
    vga[4 * 80 + 62] = 0x1f00 | '#';
    vga[5 * 80 + 60] = 0x1f00 | '#';
    vga[5 * 80 + 61] = 0x1f00 | ' ';
    vga[5 * 80 + 62] = 0x1f00 | '#';

    // N
    vga[1 * 80 + 64] = 0x1f00 | '#';
    vga[1 * 80 + 65] = 0x1f00 | ' ';
    vga[1 * 80 + 66] = 0x1f00 | '#';
    vga[2 * 80 + 64] = 0x1f00 | '#';
    vga[2 * 80 + 65] = 0x1f00 | '*';
    vga[2 * 80 + 66] = 0x1f00 | '#';
    vga[3 * 80 + 64] = 0x1f00 | '#';
    vga[3 * 80 + 65] = 0x1f00 | '@';
    vga[3 * 80 + 66] = 0x1f00 | '#';
    vga[4 * 80 + 64] = 0x1f00 | '#';
    vga[4 * 80 + 65] = 0x1f00 | ' ';
    vga[4 * 80 + 66] = 0x1f00 | '#';
    vga[5 * 80 + 64] = 0x1f00 | '#';
    vga[5 * 80 + 65] = 0x1f00 | ' ';
    vga[5 * 80 + 66] = 0x1f00 | '#';

    // -
    vga[3 * 80 + 68] = 0x1f00 | '*';
    vga[3 * 80 + 69] = 0x1f00 | '*';
    vga[3 * 80 + 70] = 0x1f00 | '*';

    // O
    vga[1 * 80 + 72] = 0x1f00 | '@';
    vga[1 * 80 + 73] = 0x1f00 | '@';
    vga[1 * 80 + 74] = 0x1f00 | '@';
    vga[2 * 80 + 72] = 0x1f00 | '#';
    vga[2 * 80 + 73] = 0x1f00 | ' ';
    vga[2 * 80 + 74] = 0x1f00 | '#';
    vga[3 * 80 + 72] = 0x1f00 | '#';
    vga[3 * 80 + 73] = 0x1f00 | ' ';
    vga[3 * 80 + 74] = 0x1f00 | '#';
    vga[4 * 80 + 72] = 0x1f00 | '#';
    vga[4 * 80 + 73] = 0x1f00 | ' ';
    vga[4 * 80 + 74] = 0x1f00 | '#';
    vga[5 * 80 + 72] = 0x1f00 | '@';
    vga[5 * 80 + 73] = 0x1f00 | '@';
    vga[5 * 80 + 74] = 0x1f00 | '@';

    // S
    vga[1 * 80 + 76] = 0x1f00 | '@';
    vga[1 * 80 + 77] = 0x1f00 | '@';
    vga[1 * 80 + 78] = 0x1f00 | '@';
    vga[2 * 80 + 76] = 0x1f00 | '#';
    vga[2 * 80 + 77] = 0x1f00 | ' ';
    vga[2 * 80 + 78] = 0x1f00 | ' ';
    vga[3 * 80 + 76] = 0x1f00 | '@';
    vga[3 * 80 + 77] = 0x1f00 | '@';
    vga[3 * 80 + 78] = 0x1f00 | '@';
    vga[4 * 80 + 76] = 0x1f00 | ' ';
    vga[4 * 80 + 77] = 0x1f00 | ' ';
    vga[4 * 80 + 78] = 0x1f00 | '#';
    vga[5 * 80 + 76] = 0x1f00 | '@';
    vga[5 * 80 + 77] = 0x1f00 | '@';
    vga[5 * 80 + 78] = 0x1f00 | '@';

    // KERNEL PANIC
    vga[7 * 80 + 44] = 0x4f00 | 'K';
    vga[7 * 80 + 45] = 0x4f00 | 'E';
    vga[7 * 80 + 46] = 0x4f00 | 'R';
    vga[7 * 80 + 47] = 0x4f00 | 'N';
    vga[7 * 80 + 48] = 0x4f00 | 'E';
    vga[7 * 80 + 49] = 0x4f00 | 'L';
    vga[7 * 80 + 50] = 0x4f00 | ' ';
    vga[7 * 80 + 51] = 0x4f00 | 'P';
    vga[7 * 80 + 52] = 0x4f00 | 'A';
    vga[7 * 80 + 53] = 0x4f00 | 'N';
    vga[7 * 80 + 54] = 0x4f00 | 'I';
    vga[7 * 80 + 55] = 0x4f00 | 'C';

    // You broke the system. Congrats. 
    vga[9 * 80 + 44] = 0x1f00 | 'Y';
    vga[9 * 80 + 45] = 0x1f00 | 'o';
    vga[9 * 80 + 46] = 0x1f00 | 'u';
    vga[9 * 80 + 47] = 0x1f00 | ' ';
    vga[9 * 80 + 48] = 0x1f00 | 'b';
    vga[9 * 80 + 49] = 0x1f00 | 'r';
    vga[9 * 80 + 50] = 0x1f00 | 'o';
    vga[9 * 80 + 51] = 0x1f00 | 'k';
    vga[9 * 80 + 52] = 0x1f00 | 'e';
    vga[9 * 80 + 53] = 0x1f00 | ' ';
    vga[9 * 80 + 54] = 0x1f00 | 't';
    vga[9 * 80 + 55] = 0x1f00 | 'h';
    vga[9 * 80 + 56] = 0x1f00 | 'e';
    vga[9 * 80 + 57] = 0x1f00 | ' ';
    vga[9 * 80 + 58] = 0x1f00 | 's';
    vga[9 * 80 + 59] = 0x1f00 | 'y';
    vga[9 * 80 + 60] = 0x1f00 | 's';
    vga[9 * 80 + 61] = 0x1f00 | 't';
    vga[9 * 80 + 62] = 0x1f00 | 'e';
    vga[9 * 80 + 63] = 0x1f00 | 'm';
    vga[9 * 80 + 64] = 0x1f00 | '.';
    vga[9 * 80 + 65] = 0x1f00 | ' ';
    vga[9 * 80 + 66] = 0x1f00 | 'C';
    vga[9 * 80 + 67] = 0x1f00 | 'o';
    vga[9 * 80 + 68] = 0x1f00 | 'n';
    vga[9 * 80 + 69] = 0x1f00 | 'g';
    vga[9 * 80 + 70] = 0x1f00 | 'r';
    vga[9 * 80 + 71] = 0x1f00 | 'a';
    vga[9 * 80 + 72] = 0x1f00 | 't';
    vga[9 * 80 + 73] = 0x1f00 | 's';
    vga[9 * 80 + 74] = 0x1f00 | '.';

    
    vga_puts(vga, 10, 44, "--------------------------------", 0x1f00);

    // Exception #num
    vga_puts(vga, 11, 44, "Exception #", 0x3f00);
    vga_putdec(vga, 11, 55, num, 0x3f00);

    const char *msg = "Unknown exception";
    if (num == 0)  msg = "Error : Division by zero";
    else if (num == 1)  msg = "Error : Debug";
    else if (num == 2)  msg = "Error : NMI";
    else if (num == 3)  msg = "Error : Breakpoint";
    else if (num == 4)  msg = "Error : Overflow";
    else if (num == 5)  msg = "Error : Bound range exceeded";
    else if (num == 6)  msg = "Error : Invalid opcode";
    else if (num == 7)  msg = "Error : Device not available";
    else if (num == 8)  msg = "Error : Double fault";
    else if (num == 10) msg = "Error : Invalid TSS";
    else if (num == 11) msg = "Error : Segment not present";
    else if (num == 12) msg = "Error : Stack segment fault";
    else if (num == 13) msg = "Error : General protection fault";
    else if (num == 14) msg = "Error : Page fault";
    else if (num == 16) msg = "Error : x87 FPU exception";
    else if (num == 17) msg = "Error : Alignment check";
    else if (num == 18) msg = "Error : Machine check";
    else if (num == 19) msg = "Error : SIMD FP exception";
    vga_puts(vga, 12, 44, msg, 0x3f00);

    vga_puts(vga, 13, 44, "EIP : ", 0x3f00);
    vga_puthex(vga, 13, 50, eip, 0x3f00);

    while(1);
}


