#include "tss.h"
#include "gdt.h"
#include <stdint.h> 
#include "com1.h"

static tss_t tss;

void tss_init() {
  tss = (tss_t) {0};
  tss.ss0 = 0x10;  
  gdt_set_tss_entry((uint32_t)&tss, sizeof(tss) - 1);
  tss.io_map_base = sizeof(tss);
  __asm__ volatile ("ltr %%ax" : : "a"(0x28));

  serial_print_hex((uint32_t)&tss);
  serial_print("\n");
  serial_print_hex(sizeof(tss));
  serial_print("\n");
  serial_print_hex(tss.ss0);
  serial_print("\n");
  serial_print_hex(tss.io_map_base);
}

