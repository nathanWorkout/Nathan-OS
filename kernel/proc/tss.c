#include "tss.h"
#include "gdt.h"
#include <stdint.h> 
#include "com1.h"

static tss_t tss;

void tss_init() { 
  gdt_set_tss_entry((uint64_t)&tss, sizeof(tss) - 1);
  tss.io_map_base = sizeof(tss);
  __asm__ volatile ("ltr %%ax" : : "a"(0x28));
}

