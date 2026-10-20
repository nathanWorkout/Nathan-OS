#include "tss.h"
#include "gdt.h"
#include <stdint.h>

// TSS placed in its own section so the linker can align it properly
__attribute__((section(".tss"))) static tss_t tss;

void tss_init() {
    tss.rsp0 = (uint64_t)(exc_stack + 8192); // kernel stack from ring 0 to ring 3 (switching)
    tss.ist1 = (uint64_t)(irq1_stack + 4096); // dedicated stack for IRQ1
    gdt_set_tss_entry((uint64_t)&tss, sizeof(tss) - 1);
    tss.io_map_base = sizeof(tss); // io_map_base = sizeof(tss) means no I/O allowed from ring 3
    __asm__ volatile ("ltr %%ax" : : "a"(0x28)); // load TSS, selector 0x28 is GDT entry 5
}

uint64_t tss_get_addr(void) {
    return (uint64_t)&tss;
}

uint64_t tss_get_size(void) {
    return sizeof(tss);
}
