#include <stdint.h>
#include "com1.h"
#include "io.h"


#define com1 0x3F8

void serial_init() {
    outb(com1 + 1, 0x00); // Couper les interrruptions
    outb(com1 + 3, 0x80); // Activer dlab pour acceder au baudrate 
    outb(com1, 0 + 115200 / 38400); // 38400 bauds donne 3
    outb(com1 + 1, 0); // octet haut (0 car 3 tient sur un octet)
    outb(com1 + 3, 0x03); // Désactive le DLAB et configure le 8N1
}

void serial_putchar(char c) {
    // Attendre que le buffer d'émission soit vide
    // On le sais grâce au bit 5
        // 0x20 car LSR retourne un octet de flag, faut pas comparer le bit entier car ca retourn 0b00100000 et 0x20 = bit 5 en hexa 
	while((inb(com1 + 5) & 0x20) == 0) {} // Attendre
	outb(com1, c);
}

void serial_print(char *str) {
    while(*str) {
	serial_putchar(*str);
    	str++;
    }
}

void serial_println(char *str) {
    serial_print(str);
    serial_print("\n");
}

void serial_print_hex(uint64_t n) {
    char hex[] = "0123456789ABCDEF";
    char buffer[19]; // 0x + 16 char + \0
    
    buffer[0] = '0';
    buffer[1] = 'x';

    for(int i = 0; i < 16; i++) {
        buffer[17 - i] = hex[n & 0xF];
        n >>= 4;
    }

    buffer[18] = '\0';

    serial_print(buffer);
}
