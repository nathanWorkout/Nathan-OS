#include "tss.h"
#include "gdt.h"
#include <stdint.h>
#include "com1.h"

static tss_t tss;

void tss_init() {
    tss.rsp0 = (uint64_t)(exc_stack + 8192);
    tss.ist1 = (uint64_t)(irq1_stack + 4096);
    gdt_set_tss_entry((uint64_t)&tss, sizeof(tss) - 1);
    tss.io_map_base = sizeof(tss);
    __asm__ volatile ("ltr %%ax" : : "a"(0x28));
}