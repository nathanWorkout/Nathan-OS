#include "pmm.h"
#include <stdint.h>
#include "com1.h"

extern char kernel_start;
extern char kernel_end;

uint32_t kstart = (uint32_t)&kernel_start; // On extrai les adresses
uint32_t kend = (uint32_t)&kernel_end;

static uint32_t bitmap[65536];

typedef struct {
    uint64_t base;  //adresse     8 octets
    uint64_t length; //           8
    uint32_t type;  // 0 ou 1 ou 2 plus tard pour reservé  4 octets
    uint32_t padding; // Car la struct fait 20 octets mais dans l'assembelur c 24 donc on rajoute 4 octets pour que ca pointe au bon endroid
} e820_entry_t;

void pmm_init(uint32_t memory_map_addr, uint32_t region_count) {
    serial_println("\nPmm init ");

    for(int i = 0; i < 65536; i++) {
	bitmap[i] = 0xffffffff;
    }

    serial_print_hex(region_count);
    serial_print("\n");

    e820_entry_t *map = (e820_entry_t *)memory_map_addr;

    uint32_t total_pages = 0;
    for(int j = 0; j < region_count; j++) {
	if(map[j].type == 1) {
	    uint32_t page_count = map[j].length / PAGE_SIZE;
	    uint32_t first_page = map[j].base / PAGE_SIZE;

	    serial_print_hex(map[j].base);
	    serial_print(" ");
	    serial_print_hex(map[j].type);
	    serial_print("\n");

	    total_pages += page_count;

	    for(uint32_t p = first_page; p < first_page + page_count; p++) { // p = index du tableau
		bitmap[p / 32] &= ~(1 << (p % 32));
	    } 
	} 
    }

    for(uint32_t kpage = kstart / PAGE_SIZE; kpage <= kend / PAGE_SIZE; kpage++) {
 	bitmap[kpage / 32] |= (1 << (kpage % 32));
    }


    serial_print("Usable pages: ");
    serial_print_hex(total_pages);
    serial_print("\n");
}


// trouver un bit à 0 le mettre à 1 retourner l'adresse
uint32_t pmm_alloc_page() {
    for(int i = 0; i < 65536; i++) {
	if(bitmap[i] == 0xffffffff) continue;

	for(int bit = 0; bit < 32; bit++) {
	    if(!(bitmap[i] & (1 << bit))) {
		bitmap[i] |= (1 << bit);

		uint32_t addr = (i * 32 + bit) * PAGE_SIZE;

		serial_print("Allocation : ");
                serial_print_hex(addr);
                serial_print("\n");

		return(i * 32 + bit) * PAGE_SIZE;
	    }
	}
    }

    serial_println("No free pages");

    return 0;
}

// mettre a 0
void pmm_free_page(uint32_t addr) {
   uint32_t page = addr / PAGE_SIZE;
   bitmap[page / 32] &= ~(1 << (page % 32));
}
