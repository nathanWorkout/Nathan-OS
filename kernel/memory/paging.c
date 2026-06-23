#include <stdint.h>
#include "paging.h"
#include "string.h"
#include "pmm.h"
#include "../limine_request.h"

int vmm_map(address_space_t *space, uint64_t virt, uint64_t phys, uint64_t flags) {
    uint64_t pml4_idx = PML4_INDEX(virt);
    uint64_t pdp_idx  = PDP_INDEX(virt);
    uint64_t pd_idx   = PD_INDEX(virt);
    uint64_t pt_idx   = PT_INDEX(virt);

    uint64_t *pml4e = &space->pml4t->entries[pml4_idx];

    if (*pml4e == 0) {
        uint64_t pdp_phys = pmm_alloc_page();
        pdp_t *new_pdp = (pdp_t *)phys_to_virt(pdp_phys);
        memset(new_pdp, 0, PAGE_SIZE);
        *pml4e = pdp_phys | PAGE_PRESENT | PAGE_RW | PAGE_USER;
    }

    pdp_t *pdp = (pdp_t *)phys_to_virt(*pml4e & ~0xFFFULL);
    uint64_t *pdpe = &pdp->entries[pdp_idx];

    if (*pdpe == 0) {
        uint64_t pd_phys = pmm_alloc_page();
        pd_t *new_pd = (pd_t *)phys_to_virt(pd_phys);
        memset(new_pd, 0, PAGE_SIZE);
        *pdpe = pd_phys | PAGE_PRESENT | PAGE_RW | PAGE_USER;
    }

    pd_t *pd = (pd_t *)phys_to_virt(*pdpe & ~0xFFFULL);
    uint64_t *pde = &pd->entries[pd_idx];

    if (*pde == 0) {
        uint64_t pt_phys = pmm_alloc_page();
        pt_t *new_pt = (pt_t *)phys_to_virt(pt_phys);
        memset(new_pt, 0, PAGE_SIZE);
        *pde = pt_phys | PAGE_PRESENT | PAGE_RW | PAGE_USER;
    }

    pt_t *pt = (pt_t *)phys_to_virt(*pde & ~0xFFFULL);
    pt->entries[pt_idx] = phys | flags;
    asm volatile("invlpg (%0)" :: "r"(virt) : "memory");

    return 0;
}

		int vmm_unmap(address_space_t *space, uint64_t virt) {
    uint64_t pml4_idx = PML4_INDEX(virt);
    uint64_t pdp_idx  = PDP_INDEX(virt);
    uint64_t pd_idx   = PD_INDEX(virt);
    uint64_t pt_idx   = PT_INDEX(virt);

    uint64_t *pml4e = &space->pml4t->entries[pml4_idx];
    if (!(*pml4e & PAGE_PRESENT)) return -1;

    pdp_t *pdp = (pdp_t *)phys_to_virt(*pml4e & ~0xFFFULL);
    uint64_t *pdpe = &pdp->entries[pdp_idx];
    if (!(*pdpe & PAGE_PRESENT)) return -1;

    pd_t *pd = (pd_t *)phys_to_virt(*pdpe & ~0xFFFULL);
    uint64_t *pde = &pd->entries[pd_idx];
    if (!(*pde & PAGE_PRESENT)) return -1;

    pt_t *pt = (pt_t *)phys_to_virt(*pde & ~0xFFFULL);
    uint64_t *pte = &pt->entries[pt_idx];
    if (!(*pte & PAGE_PRESENT)) return -1;

    uint64_t phys = *pte & ~0xFFFULL;
    *pte = 0;

    asm volatile("invlpg (%0)" :: "r"(virt) : "memory");
    pmm_free_page(phys);

    return 0;
}

static address_space_t kernel_space_static;
address_space_t *vmm_create_kernel_space(void) {
    uint64_t pml4_phys = pmm_alloc_page();
    pml4_t *new_pml4 = (pml4_t *)phys_to_virt(pml4_phys);
    memset(new_pml4, 0, PAGE_SIZE);
    kernel_space_static.pml4t = new_pml4;
    kernel_space_static.pml4_phys = pml4_phys;
    kernel_space_static.pid = 0;
    kernel_space_static.ref_count = 1;

    return &kernel_space_static;
}
