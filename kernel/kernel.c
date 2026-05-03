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
#include "sqrt.h"
#include "sdf.h"
#include "rgba.h"
#include "font.h"
#include "kernel_panic.h"

void kmain(void) {
    gdt_init();
    idt_init();
    isr_init();
    serial_init();
    pic_init();
    pit_init(1000);
    tss_init();
    
    Canvas screen = fb_get_canvas();
    gfx_init(&screen);
    tty_init(screen);  
    //volatile int x = 1 / 0;

    __asm__ volatile ("sti");
    pic_clear_mask(0); 
    pic_clear_mask(1); 
    shell_run();
    while (1);
}
