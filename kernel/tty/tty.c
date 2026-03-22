#include <stdint.h>

#define vga_width 80

static int cursor_x = 0;
static int cursor_y = 0;
static unsigned char couleur = 0x1f;

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "nd"(port)); // val dans eax et port dans dx e envoie eax -> dx
}

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "nd"(port)); // lit l'octet depuis le port hardware et fait int eax, val
    return val;
}

// 0x3d4 : port index ; 0x3d5 port data : lire et écrire epuis le registre
void enable_cursor(uint8_t cursor_start, uint8_t cursor_end) {
    outb(0x3d4, 0x0a); 
    outb(0x3d5, (inb(0x3d5) & 0xc0) | cursor_start); // on veut le registre qui controle le début du curseur et écrit
    outb(0x3d4, 0x0b); 
    outb(0x3d5, (inb(0x3d5) & 0xe0) | cursor_end); // 0x0e car on a 3 bits a préserver
}

void update_cursor(int x, int y) {
    uint16_t pos = y * vga_width + x;
    outb(0x3d4, 0x0f);				
    outb(0x3d5, (uint8_t)(pos & 0xff)); // garde seulement les bits de droite et on envoie au vga  
    outb(0x3d4, 0x0e);
    outb(0x3d5, (uint8_t)((pos >> 8) & 0xff)); // décale tout vers la dorite qu'on envoie au vga et il recolle les 2
}

// lit les 8 bits de poid fort et 8 de poid faible pour reformer la position de 16 bits
uint16_t get_cursor_position(void) {
    uint16_t pos = 0; // on lit les 8 bits de poid faible et on les met dans pos
    outb(0x3d4, 0x0f);
    pos |= inb(0x3d5);// on lit les 8 bits de poid fort
    outb(0x3d4, 0x0e);
    pos |= ((uint16_t)inb(0x3d5)) << 8;// inb lit un octet depuis le port hardware

    return pos;
}

void tty_init() {
    enable_cursor(13, 15); // curseur bas
			   
    cursor_x = 0;
    cursor_y = 0;

    volatile unsigned short *vga = (unsigned short *) 0xb8000;

    for(int i = 0; i < 80 * 25; i++){
	vga[i] = (couleur << 8) | ' '; // signé 8 bits car 0-7 : caractère, 8-15 : couleur
    }

    update_cursor(cursor_x, cursor_y);
}

void putchar(char c) {
    volatile unsigned short *vga = (unsigned short *) 0xb8000;

    switch(c) {
	case '\n': cursor_y++; cursor_x = 0; break; // entrée
	case '\r': cursor_x = 0; break; // certains terminaux envoient \r\n ensemble pour un retour à la ligne (convention windows ou dos) 
	case '\t': cursor_x += 4; break; // tab (4 espaces parce que c'est plus pratique pour coder :)
	default:
	    vga[cursor_y * 80 + cursor_x] = (couleur <<8) | c;
	    cursor_x++;
    }

    if(cursor_x >= 80) {
	cursor_x = 0;
	cursor_y++;
    }

    // on parcours toutes les lignes, puis on les décale de - 1 et on efface la premiere
    if(cursor_y >= 25) {
	for(int a = 1; a <= 24; a++) {
	    for(int b = 0; b <= 79; b++) {
		vga[(a - 1) * 80 + b] = vga[a * 80 + b];
	    }
	}

	for(int b = 0; b <= 79; b++) {
	    vga[24 * 80 + b] = (couleur << 8) | ' ';
	}
        cursor_y = 24;
	cursor_x = 0;
    }

    update_cursor(cursor_x, cursor_y);
}

void puts(char *s) {
    while(*s != '\0') {
	putchar(*s);
	s++
    }
    putchar('\n');
}

void printk() {
    
}
