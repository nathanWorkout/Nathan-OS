#ifndef COM1_H
#define COM1_H

void serial_init();
void serial_putchar(char c);
void serial_print(char *str);
void serial_println(char *str);
void serial_print_hex(uint32_t n); 

#endif
