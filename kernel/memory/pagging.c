#include <stdint.h>
#include "com1.h" 
#include "io.h"
#include "pmm.h"

uint32_t *pagging_create_page_directory();
uint32_t *pagging_create_table();
void pagging_map_page_table(uint32_t *pd, uint32_t *pt);
void pagging_enable(uint32_t *pd);

void pagging_init() {
  asm volatile("cli");  // Si pas de cli ni sti, cr0 et cr3 est probabiliste
  uint32_t *pd = pagging_create_page_directory();
  uint32_t *pt = pagging_create_table();
  pagging_map_page_table(pd, pt);
  pagging_enable(pd);
  asm volatile("sti");
}

uint32_t *pagging_create_page_directory() {
  uint32_t *page_directory = (uint32_t *)pmm_alloc_page();
  
  for(int i = 0; i < 1024; i++) {
    page_directory[i] = 0x0;  
  }

  return page_directory;
}

uint32_t *pagging_create_table() {
  uint32_t *pt = (uint32_t *)pmm_alloc_page();

  for(int i = 0; i < 1024; i++) {
    pt[i] = (i * 0x1000) | 0b011;  // Present ; R/W = true ; User/Supervisor = 0 : kernel only
  }

  return pt;
}

// On mappe les 4 premiers MO pour proteger le bios, kernel...
void pagging_map_page_table(uint32_t *pd, uint32_t *pt) {
  pd[0] = (uint32_t)pt | 0b011;  
}

void pagging_enable(uint32_t *pd) {
  serial_println("Pagging enable");

  serial_print("Chargement cr3...");
  asm volatile("mov %0, %%cr3" :: "r"(pd));
  serial_println("cr3 ok");
  serial_println("Activation cr0");
  asm volatile(
    "mov %%cr0, %%eax\n"
    "or $0x80000000, %%eax\n"
    "mov %%eax, %%cr0\n"
    ::: "eax" // Pas modifier eax
  );
  serial_println("cr0 ok");
}
