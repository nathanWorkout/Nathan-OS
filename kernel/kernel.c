#include "idt.h"
#include "isr.h"
#include "tty.h"
// tty c TeleTypeWritter c stylé comme nom
#include "com1.h"
#include "pic8089.h"
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
    serial_print("Hello, world of debug !");

    tty_init();
    printk("Hello, World !");

    pit_init(1000);
    
    pic_init();
    __asm__ volatile ("sti");   

    pic_clear_mask(0);
   

    while (1); 
}
