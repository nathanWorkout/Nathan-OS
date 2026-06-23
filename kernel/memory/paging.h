#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

#define PAGE_SIZE          4096
#define PAGE_PRESENT       (1ULL << 0)
#define PAGE_RW            (1ULL << 1)
#define PAGE_USER          (1ULL << 2)
#define PAGE_WRITE_THROUGH (1ULL << 3)
#define PAGE_CACHE_DISABLE (1ULL << 4)
#define PAGE_ACCESSED      (1ULL << 5)
#define PAGE_DIRTY         (1ULL << 6)
#define PAGE_COW           (1ULL << 7)
#define PAGE_GLOBAL        (1ULL << 8)

#define PML4_INDEX(virt)   (((virt) >> 39) & 0x1FF)
#define PDP_INDEX(virt)    (((virt) >> 30) & 0x1FF)
#define PD_INDEX(virt)     (((virt) >> 21) & 0x1FF)
#define PT_INDEX(virt)     (((virt) >> 12) & 0x1FF)

typedef struct {
    uint64_t entries[512];
} __attribute__((aligned(4096))) pml4_t;

typedef struct {
    uint64_t entries[512];
} __attribute__((aligned(4096))) pdp_t;

typedef struct {
    uint64_t entries[512];
} __attribute__((aligned(4096))) pd_t;

typedef struct {
    uint64_t entries[512];
} __attribute__((aligned(4096))) pt_t;

typedef struct {
    pml4_t   *pml4t;
    uint64_t  pml4_phys;
    uint64_t  pid;
    uint64_t  ref_count;
} address_space_t;

int vmm_map(address_space_t *space, uint64_t virt, uint64_t phys, uint64_t flags);
int vmm_unmap(address_space_t *space, uint64_t virt);
address_space_t *vmm_create_kernel_space(void);

#endif
