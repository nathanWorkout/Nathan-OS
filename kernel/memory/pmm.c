#include "pmm.h"
#include <stdint.h>
#include "../serial/com1.h"
#include "limine.h"
#include "paging.h"
#include "../limine_request.h"
#include "string.h"

extern char kernel_phys_start;
extern char kernel_phys_end;

#define BITMAP_SIZE 4096
#define MAX_PAGES   (BITMAP_SIZE * 64)

static uint64_t bootstrap_bitmap[BITMAP_SIZE];
static int      bootstrap_active = 1;  // Tant que le pmm est actif

static inline int  bs_test (uint64_t p) { return (bootstrap_bitmap[p/64] >> (p%64)) & 1; }
static inline void bs_set  (uint64_t p) { bootstrap_bitmap[p/64] |=  (1ULL << (p%64)); }
static inline void bs_clear(uint64_t p) { bootstrap_bitmap[p/64] &= ~(1ULL << (p%64)); }

static uint64_t *full_bitmap       = NULL;
static uint64_t  full_bitmap_pages = 0;

#define MAX_BITMAP_PAGES 64
static uint64_t bitmap_phys_pages[MAX_BITMAP_PAGES];
static uint64_t bitmap_page_count = 0;

void pmm_init(struct limine_memmap_response *memmap) {
    for (int i = 0; i < BITMAP_SIZE; i++) {
        bootstrap_bitmap[i] = 0xFFFFFFFFFFFFFFFFULL;
    }

    uint64_t kstart = (uint64_t)&kernel_phys_start;
    uint64_t kend   = (uint64_t)&kernel_phys_end;
    uint64_t total_pages = 0;

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type != LIMINE_MEMMAP_USABLE) continue;

        uint64_t base   = entry->base;
        uint64_t length = entry->length;

        if (base >= MAX_PAGES * PAGE_SIZE) continue;
        if (base + length > MAX_PAGES * PAGE_SIZE)
            length = MAX_PAGES * PAGE_SIZE - base;

        uint64_t first_page = base / PAGE_SIZE;
        uint64_t count      = length / PAGE_SIZE;

        for (uint64_t p = first_page; p < first_page + count; p++) {
            bs_clear(p);
            total_pages++;
        }
    }

    uint64_t kp_start = kstart / PAGE_SIZE;
    uint64_t kp_end   = (kend + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint64_t p = kp_start; p < kp_end; p++)
        bs_set(p);

    bs_set(0);

}

void pmm_init_full(struct limine_memmap_response *memmap,address_space_t *space) {
    uint64_t max_addr = 0;
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *e = memmap->entries[i];
        if (e->type != LIMINE_MEMMAP_USABLE) continue;
        uint64_t end = e->base + e->length;
        if (end > max_addr) max_addr = end;
    }

    uint64_t total_pages  = max_addr / PAGE_SIZE;
    uint64_t bitmap_bytes = (total_pages + 7) / 8;
    bitmap_page_count     = (bitmap_bytes + PAGE_SIZE - 1) / PAGE_SIZE;


    if (bitmap_page_count > MAX_BITMAP_PAGES) {
        serial_print("[PMM] FATAL: bitmap too large\n");
        return;
    }

    for (uint64_t i = 0; i < bitmap_page_count; i++) {
        uint64_t phys = pmm_alloc_page();
        if (phys == 0) {
            serial_print("[PMM] FATAL: plus de pages pour la bitmap\n");
            return;
        }
        bitmap_phys_pages[i] = phys;
    }

    uint64_t bitmap_virt = phys_to_virt(bitmap_phys_pages[0]);
    for (uint64_t i = 0; i < bitmap_page_count; i++) {
        uint64_t virt = phys_to_virt(bitmap_phys_pages[i]);
        vmm_map(space, virt, bitmap_phys_pages[i], PAGE_PRESENT | PAGE_RW);
    }
    full_bitmap       = (uint64_t *)bitmap_virt;
    full_bitmap_pages = total_pages;

    memset(full_bitmap, 0xFF, bitmap_bytes);

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *e = memmap->entries[i];
        if (e->type != LIMINE_MEMMAP_USABLE) continue;
        uint64_t first = e->base / PAGE_SIZE;
        uint64_t count = e->length / PAGE_SIZE;
        for (uint64_t p = first; p < first + count; p++) {
            if (p < full_bitmap_pages)
                full_bitmap[p/64] &= ~(1ULL << (p%64));
        }
    }

    for (uint64_t i = 0; i < bitmap_page_count; i++) {
        uint64_t page = bitmap_phys_pages[i] / PAGE_SIZE;
        if (page < full_bitmap_pages) {
            full_bitmap[page/64] |= (1ULL << (page%64));
	}
    }

}

void pmm_switch_to_full(void) {
    if (!full_bitmap) {
        //serial_print("[PMM] WARN: pmm_switch_to_full appelé sans full_bitmap\n");
        return;
    }

    bootstrap_active = 0;

    memset(bootstrap_bitmap, 0xFF, sizeof(bootstrap_bitmap));
}

uint64_t pmm_alloc_page(void) {
    if (full_bitmap) {
        // Full PMM
        uint64_t size = (full_bitmap_pages + 63) / 64;
        for (uint64_t i = 0; i < size; i++) {
            if (full_bitmap[i] == 0xFFFFFFFFFFFFFFFFULL) continue;
            for (int bit = 0; bit < 64; bit++) {
                if (!(full_bitmap[i] & (1ULL << bit))) {
                    full_bitmap[i] |= (1ULL << bit);
                    return (i * 64 + bit) * PAGE_SIZE;
                }
            }
        }
        serial_print("[PMM] ERREUR: plus de pages physiques (full)\n");
        return 0;
    }

    if (!bootstrap_active) {
        serial_print("[PMM] ERREUR: bootstrap detruit, full PMM non pret\n");
        return 0;
    }

    // Bootstrap PMM
    for (int i = 0; i < BITMAP_SIZE; i++) {
        if (bootstrap_bitmap[i] == 0xFFFFFFFFFFFFFFFFULL) continue;
        for (int bit = 0; bit < 64; bit++) {
            if (!(bootstrap_bitmap[i] & (1ULL << bit))) {
                bootstrap_bitmap[i] |= (1ULL << bit);
                return (uint64_t)(i * 64 + bit) * PAGE_SIZE;
            }
        }
    }
    serial_print("[PMM] ERREUR: plus de pages physiques (bootstrap)\n");
    return 0;
}

void pmm_free_page(uint64_t addr) {
    if (addr & (PAGE_SIZE - 1)) {
        serial_print("[PMM] ERREUR: free adresse non alignee 0x");
        serial_print_hex(addr);
        serial_print("\n");
        return;
    }

    uint64_t page = addr / PAGE_SIZE;

    if (full_bitmap) {
        if (page >= full_bitmap_pages) {
            serial_print("[PMM] ERREUR: free hors plage 0x");
            serial_print_hex(addr);
            serial_print("\n");
            return;
        }
        if (!(full_bitmap[page/64] & (1ULL << (page%64)))) {
            serial_print("[PMM] WARN: double free 0x");
            serial_print_hex(addr);
            serial_print("\n");
            return;
        }
        full_bitmap[page/64] &= ~(1ULL << (page%64));
        return;
    }

    if (page >= MAX_PAGES) {
        serial_print("[PMM] ERREUR: free hors plage 0x");
        serial_print_hex(addr);
        serial_print("\n");
        return;
    }
    if (!bs_test(page)) {
        serial_print("[PMM] WARN: double free 0x");
        serial_print_hex(addr);
        serial_print("\n");
        return;
    }
    bs_clear(page);
}

uint64_t pmm_alloc_pages_contig(uint64_t count) {
    if (count == 0) return 0;

   uint64_t *bitmap;
    uint64_t total;

    if (full_bitmap) {
        bitmap = full_bitmap;
        total = full_bitmap_pages;
    } else {
        bitmap = bootstrap_bitmap;
        total = MAX_PAGES;
    }

    uint64_t run = 0;
    uint64_t run_start = 0;

    for (uint64_t p = 1; p < total; p++) {
        if (!(bitmap[p/64] & (1ULL << (p%64)))) {
            if (run == 0) run_start = p;
            run++;
            if (run == count) {
                for (uint64_t i = run_start; i < run_start + count; i++) {
                    bitmap[i/64] |= (1ULL << (i%64));
                }
                return run_start * PAGE_SIZE;
            }
        } else {
            run = 0;
        }
    }

    return 0;
}

void pmm_free_pages_contig(uint64_t phys_addr, uint64_t count) {
    uint64_t start = phys_addr / PAGE_SIZE;
    for (uint64_t i = start; i < start + count; i++) {
        pmm_free_page(i * PAGE_SIZE);
    }
}
