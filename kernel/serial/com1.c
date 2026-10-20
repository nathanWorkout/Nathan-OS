#include <stdint.h>
#include "com1.h"
#include "io.h"

#define com1 0x3F8

void serial_init() {
    outb(com1 + 1, 0x00); /* disable interrupts */
    outb(com1 + 3, 0x80); /* enable DLAB to access baudrate registers */
    outb(com1, 0 + 115200 / 38400); /* 38400 baud → divisor = 3 */
    outb(com1 + 1, 0);    /* high byte of divisor (0, divisor fits in one byte) */
    outb(com1 + 3, 0x03); /* disable DLAB, set 8N1 (8 bits, no parity, 1 stop bit) */
}

void serial_putchar(char c) {
    /* wait until transmit buffer is empty (LSR bit 5 = 0x20) */
    while ((inb(com1 + 5) & 0x20) == 0) {}
    outb(com1, c);
}

void serial_print(char *str) {
    while (*str) {
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
    char buffer[19]; /* 0x + 16 hex digits + \0 */

    buffer[0] = '0';
    buffer[1] = 'x';

    for (int i = 0; i < 16; i++) {
        buffer[17 - i] = hex[n & 0xF];
        n >>= 4;
    }

    buffer[18] = '\0';

    serial_print(buffer);
}
