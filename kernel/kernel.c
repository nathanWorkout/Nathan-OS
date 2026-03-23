#include "idt.h"
#include "isr.h"
#include "tty.h"
#include "com1.h"
// tty c TeleTypeWritter c stylé comme nom

#include <stdint.h>

void kmain(void) {

    idt_init();
    isr_init();

/*
    volatile int a = 10;
    volatile int b = 0;
    volatile int c = a / b;
 */

    serial_init();
    serial_print("Hello, World of debug !");

    tty_init();
    printk("Hello, World !");

   

    while (1); 
}
