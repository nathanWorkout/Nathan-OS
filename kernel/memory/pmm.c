#include "pmm.h"
#include <stdint.h>
#include "../serial/com1.h"
#include "limine.h"

extern char kernel_phys_start;
extern char kernel_phys_end;

#define BITMAP_SIZE 4096
#define MAX_PAGES   (BITMAP_SIZE * 64)

static uint64_t bitmap[BITMAP_SIZE];

static inline int bitmap_test(uint64_t page) {
    if (page >= MAX_PAGES) return 0;
    return (bitmap[page / 64] >> (page % 64)) & 1;
}

static inline void bitmap_set(uint64_t page) {
    if (page < MAX_PAGES) bitmap[page / 64] |= (1ULL << (page % 64));
}

static inline void bitmap_clear(uint64_t page) {
    if (page < MAX_PAGES)
        bitmap[page / 64] &= ~(1ULL << (page % 64));
}

void pmm_init(struct limine_memmap_response *memmap) {
    for (int i = 0; i < BITMAP_SIZE; i++)
        bitmap[i] = 0xFFFFFFFFFFFFFFFFULL;

    uint64_t kstart = (uint64_t)&kernel_phys_start;
    uint64_t kend   = (uint64_t)&kernel_phys_end;
    uint64_t total_pages = 0;

    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        if (entry->type != LIMINE_MEMMAP_USABLE)
            continue;

        uint64_t base = entry->base;
        uint64_t length = entry->length;

        if (base >= MAX_PAGES * PAGE_SIZE) {
            continue;
        }
        if (base + length > MAX_PAGES * PAGE_SIZE) {
            length = MAX_PAGES * PAGE_SIZE - base;
        }

        uint64_t first_page = base / PAGE_SIZE;
        uint64_t page_count = length / PAGE_SIZE;

        for (uint64_t p = first_page; p < first_page + page_count; p++) {
            bitmap_clear(p);
            total_pages++;
        }
    }

    uint64_t kpage_start = kstart / PAGE_SIZE;
    uint64_t kpage_end   = (kend + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint64_t p = kpage_start; p < kpage_end; p++)
        bitmap_set(p);

    bitmap_set(0);

    serial_print("[PMM] Initialised. Usable pages: ");
    serial_print_hex(total_pages);
    serial_print("\n");
}

uint64_t pmm_alloc_page(void) {
    for (int i = 0; i < BITMAP_SIZE; i++) {
        if (bitmap[i] == 0xFFFFFFFFFFFFFFFFULL) continue;
        for (int bit = 0; bit < 64; bit++) {
            if (!(bitmap[i] & (1ULL << bit))) {
                bitmap[i] |= (1ULL << bit);
                uint64_t addr = ((uint64_t)i * 64 + bit) * PAGE_SIZE;
                return addr;
            }
        }
    }
    return 0;
}

void pmm_free_page(uint64_t addr) {
    if (addr & (PAGE_SIZE - 1)) {
        serial_print("[PMM] ERROR: free unaligned address 0x");
        serial_print_hex(addr);
        serial_print("\n");
        return;
    }

    uint64_t page = addr / PAGE_SIZE;

    if (page >= MAX_PAGES) {
        serial_print("[PMM] ERROR: free out-of-range address 0x");
        serial_print_hex(addr);
        serial_print("\n");
        return;
    }

    if (!bitmap_test(page)) {
        serial_print("[PMM] WARNING: double free at 0x");
        serial_print_hex(addr);
        serial_print("\n");
        return;
    }

    bitmap_clear(page);
}
