#ifndef PMM_H
#define PMM_H
#include <stdint.h>

#define PAGE_SIZE 4096 // Une page fait 4ko -> 1024 * 4

void pmm_init(uint64_t memory_map_addr, uint64_t region_count);
void pmm_free_page(uint64_t addr);
uint64_t pmm_alloc_page(); // Un type qui retourne 32 bits car on a besoin de retourner une adresse 32 bits

#endif
