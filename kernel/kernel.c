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

extern gdt_entry_t gdt[7];
extern idt_entry_t idt[256];

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
    serial_print("[DEBUG] avant vmm_apply_nx\n");
    vmm_apply_nx(kernel_space);
    serial_print("[DEBUG] apres vmm_apply_nx\n");
    serial_print("avant switch\n");
    vmm_switch_space(kernel_space);
    serial_print("apres switch\n");
    pmm_switch_to_full();            serial_print("1\n");
    enable_wp();                     serial_print("2\n");
    pic_init();                      serial_print("3\n");
    pit_init(1000);                  serial_print("4\n");
    tss_init();                      serial_print("5\n");
    vmm_set_readonly(kernel_space, (uint64_t)idt, sizeof(idt));   serial_print("6\n");
    vmm_set_readonly(kernel_space, (uint64_t)gdt, sizeof(gdt_entry_t) * GDT_ENTRY_COUNT); serial_print("7\n");
    vmm_set_readonly(kernel_space, tss_get_addr(), tss_get_size()); serial_print("8\n");

    #if 0
    serial_print("[TEST] Tentative d'ecriture sur IDT (doit fault)...\n");
    volatile uint8_t *test = (volatile uint8_t *)idt;
    *test = 0xFF;
    serial_print("[TEST] ERREUR: l'ecriture a reussi, RO ne fonctionne pas !\n");
    #endif

    serial_print("a\n");
    Canvas screen = fb_get_canvas();
    serial_print("b\n");
    gfx_init(&screen);
    serial_print("c\n");
    tty_init(screen);
    serial_print("d\n");

    __asm__ volatile ("sti");
    serial_print("e\n");
    pic_clear_mask(0);
    serial_print("f\n");
    pic_clear_mask(1);
    serial_print("g\n");

    Canvas cv = fb_get_canvas();
    serial_print("h\n");
    shell_run(&cv);
    serial_print("i\n");

    while (1) {
        __asm__("hlt");
    }
}
