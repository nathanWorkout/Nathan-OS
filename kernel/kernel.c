#include "idt.h"
#include "idt/gdt.h"
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
#include "gdt.h"
#include "ring_buffer.h"
#include "tss.h"
#include "gfx.h"
#include "2d_renderer.h"
#include "framebuffer.h"
#include "ssaa.h"


//extern char kernel_stack_top[];

void kmain(void) {
 //   serial_print_hex((uint32_t)&kernel_stack_top);
    gdt_init();
    idt_init();
    isr_init();
  
     

    serial_init(); 
    tty_init();
//    pmm_init(0x6000, *(uint16_t*)0x5FFE); // Adresse de la mémory map et le nombre de région
//    pagging_init();
    pic_init();
    pit_init(1000);
    __asm__ volatile ("sti");   
    pic_clear_mask(1);
    uint8_t mask = inb(0x21);
    serial_print_hex(mask);
    tss_init();
    
//    shell_run();
    void gfx_init(Canvas *cv);
    Canvas screen = fb_get_canvas();
    gfx_init(&screen);
  //  volatile int a = 1 / 0;
    while (1); 
}

