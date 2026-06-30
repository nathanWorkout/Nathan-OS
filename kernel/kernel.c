#include "idt/idt.h"
#include "idt/gdt.h"
#include "idt/isr.h"
#include "serial/com1.h"
#include "tty/tty.h"
#include "com1.h"
#include "pic/pic8089.h"
#include "pit/pit.h"
#include <stdint.h>
#include "memory/pmm.h"
#include "memory/paging.h"
#include "gdt.h"
#include "ring_buffer.h"
#include "proc/tss.h"
#include "Graphic/gfx.h"
#include "Graphic/2d_renderer.h"
#include "Graphic/framebuffer.h"
#include "Graphic/ssaa.h"
#include "sqrt.h"
#include "sdf.h"
#include "rgba.h"
#include "font.h"
#include "kernel_panic.h"
#include "limine_request.h"
#include <stddef.h>

void kmain(void) {
    gdt_init();
    idt_init();
    isr_init();
    serial_init();

    enable_nxe();

    pmm_init(memmap_request.response);

    address_space_t *kernel_space = vmm_create_kernel_space();
    if (!kernel_space) {
        serial_print("[KERNEL] FATAL: Failed to create kernel space\n");
        for (;;);
    }

    pmm_init_full(memmap_request.response, kernel_space);
    vmm_apply_nx(kernel_space);

    vmm_switch_space(kernel_space);
    pmm_switch_to_full();

    enable_wp();

    pic_init();
    pit_init(1000);
    tss_init();

    Canvas screen = fb_get_canvas();
    gfx_init(&screen);
    tty_init(screen);

    __asm__ volatile ("sti");
    pic_clear_mask(0);
    pic_clear_mask(1);

    Canvas cv = fb_get_canvas();
    shell_run(&cv);

    while (1) {
        __asm__("hlt");
    }
}
