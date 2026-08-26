#include "kmalloc.h"
#include "pmm.h"
#include "paging.h"
#include "string.h"

typedef struct BlockHeader {
    uint64_t size;
    uint64_t pages;
    uint64_t magic;
    struct BlockHeader *next;
} BlockHeader;

#define KMALLOC_MAGIC 0xDEADFA11ULL
#define HEADER_SIZE   sizeof(BlockHeader)

void *kmalloc(uint64_t size) {
    if (size == 0) return NULL;

    uint64_t total = size + HEADER_SIZE;
    uint64_t pages = (total + PAGE_SIZE - 1) / PAGE_SIZE;

    uint64_t phys = pmm_alloc_pages_contig(pages);
    if (!phys) return NULL;

    uint64_t virt = phys_to_virt(phys);
    memset((void *)virt, 0, pages * PAGE_SIZE);

    BlockHeader *hdr = (BlockHeader *)virt;
    hdr->size  = size;
    hdr->pages = pages;
    hdr->magic = KMALLOC_MAGIC;
    hdr->next  = NULL;

    return (void *)(virt + HEADER_SIZE);
}

void kfree(void *ptr) {
    if (!ptr) return;

    BlockHeader *hdr = (BlockHeader *)((uint64_t)ptr - HEADER_SIZE);

    if (hdr->magic != KMALLOC_MAGIC) {
        // corruption détectée, on touche à rien
        return;
    }

    hdr->magic = 0; // invalide le bloc
    pmm_free_pages_contig(virt_to_phys((uint64_t)hdr), hdr->pages);
}
