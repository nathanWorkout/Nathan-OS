#include "idt.h"
#include "isr.h"
#include "memory/pagging.h"
#include "serial/com1.h"
#include "tty.h"
// tty c TeleTypeWritter c stylé comme nom
#include "com1.h"
#include "pic8089.h"
#include "pit.h"
#include <stdint.h>
#include "pmm.h"
#include "pagging.h"

void kmain(void) {

    idt_init();
    isr_init();

/*
    volatile int a = 10;
    volatile int b = 0;
    volatile int c = a / b;
*/

    serial_init(); 

    tty_init();


    pmm_init(0x6000, *(uint16_t*)0x5FFE); // Adresse de la mémory map et le nombre de région
    pagging_init();
    serial_println("La pagination s'est bien passé !");

    printk("Hello, World");

    pit_init(1000);
    
    pic_init();
    __asm__ volatile ("sti");   

    pic_clear_mask(0);

    
    asm volatile("cli"); while(1);
    while (1); 
}
