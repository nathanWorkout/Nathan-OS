#pragma once
#include <stdint.h>
#include "limine.h"
#include "paging.h"

void     pmm_init(struct limine_memmap_response *memmap);
void     pmm_init_full(struct limine_memmap_response *memmap, address_space_t *space);
void     pmm_switch_to_full(void);
uint64_t pmm_alloc_page(void);
void     pmm_free_page(uint64_t phys_addr);
uint64_t pmm_alloc_pages_contig(uint64_t count);
void     pmm_free_pages_contig(uint64_t phys_addr, uint64_t count);
