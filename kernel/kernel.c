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
#include "kmalloc.h"
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
#include "VaultFs/vault_global.h"
#include "limine_request.h"
#include "../kernel/Graphic/gui/wallpaper_loader/png.h"
#include "../kernel/drivers/mouse/mouse.h"
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
    serial_print("[KERNEL] Failed to create kernel space\n");
    for (;;);
    }
    pmm_init_full(memmap_request.response, kernel_space);
    vmm_apply_nx(kernel_space);
    Canvas screen = fb_get_canvas();
    vmm_map_framebuffer(kernel_space, &screen);
    vmm_switch_space(kernel_space);
    pmm_switch_to_full();
    enable_wp();
    pic_init();
    pit_init(1000);
    tss_init();
    //vmm_set_readonly(kernel_space, (uint64_t)idt, sizeof(idt));
    vmm_set_readonly(kernel_space, (uint64_t)gdt, sizeof(gdt_entry_t) * GDT_ENTRY_COUNT);
    vmm_set_readonly(kernel_space, tss_get_addr(), tss_get_size());

    #if 0
        serial_print("[TEST] Tentative d'ecriture sur IDT ...\n");
        volatile uint8_t *test = (volatile uint8_t *)idt;
        *test = 0xFF;
        serial_print("[TEST] ERREUR: l'ecriture a reussi, RO ne fonctionne pas !\n");
    #endif

    gfx_init(&screen);
    tty_init(screen);
    __asm__ volatile ("sti");
    pic_clear_mask(0);
    pic_clear_mask(1);
    Canvas cv = fb_get_canvas();
    vaultfs_init(&g_vaultfs);
    //vmm_set_readonly(kernel_space, (uint64_t)&g_vaultfs.layer1, sizeof(VaultIndex));
    //mouse_init();
    //pic_clear_mask(12);
    shell_run(&cv);
    while (1) {
    __asm__("hlt");
    }
}