#include <stdint.h>
#include "paging.h"
#include "string.h"
#include "pmm.h"
#include "com1.h"
#include "Graphic/framebuffer.h"
#include "../limine_request.h"

extern char _text_start,   _text_end;
extern char _rodata_start, _rodata_end;
extern char _data_start,   _data_end;
extern char _bss_start, _bss_end;

static inline uint64_t kernel_virt_to_phys(uint64_t virt) {
    if (kernel_address_request.response == NULL) return virt;
    uint64_t virt_base = kernel_address_request.response->virtual_base;
    uint64_t phys_base = kernel_address_request.response->physical_base;
    return virt - virt_base + phys_base;
}

int vmm_map(address_space_t *space, uint64_t virt, uint64_t phys, uint64_t flags) {
    uint64_t pml4_idx    = PML4_INDEX(virt);
    uint64_t pdp_idx     = PDP_INDEX(virt);
    uint64_t pd_idx      = PD_INDEX(virt);
    uint64_t pt_idx      = PT_INDEX(virt);
    uint64_t table_flags = PAGE_PRESENT | PAGE_RW | (flags & PAGE_USER);

    uint64_t *pml4e = &space->pml4t->entries[pml4_idx];
    if (!(*pml4e & PAGE_PRESENT)) {
        uint64_t pdp_phys = pmm_alloc_page();
        if (!pdp_phys) return -1;
        memset((void *)phys_to_virt(pdp_phys), 0, PAGE_SIZE);
        *pml4e = pdp_phys | table_flags;
    }
    pdp_t *pdp = (pdp_t *)phys_to_virt(*pml4e & ~0xFFFULL);

    uint64_t *pdpe = &pdp->entries[pdp_idx];
    if (!(*pdpe & PAGE_PRESENT)) {
        uint64_t pd_phys = pmm_alloc_page();
        if (!pd_phys) return -1;
        memset((void *)phys_to_virt(pd_phys), 0, PAGE_SIZE);
        *pdpe = pd_phys | table_flags;
    }
    pd_t *pd = (pd_t *)phys_to_virt(*pdpe & ~0xFFFULL);

    uint64_t *pde = &pd->entries[pd_idx];
    if (!(*pde & PAGE_PRESENT)) {
        uint64_t pt_phys = pmm_alloc_page();
        if (!pt_phys) return -1;
        memset((void *)phys_to_virt(pt_phys), 0, PAGE_SIZE);
        *pde = pt_phys | table_flags;
    }
    pt_t *pt = (pt_t *)phys_to_virt(*pde & ~0xFFFULL);

    pt->entries[pt_idx] = phys | flags;

    asm volatile("invlpg (%0)" :: "r"(virt) : "memory");
    return 0;
}

static int table_is_empty(uint64_t *entries, int count)
{
    for (int i = 0; i < count; i++)
        if (entries[i]) return 0;
    return 1;
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

    if (table_is_empty(pt->entries, 512)) {
        uint64_t pt_phys = *pde & ~0xFFFULL;
        *pde = 0;
        pmm_free_page(pt_phys);

        if (table_is_empty(pd->entries, 512)) {
            uint64_t pd_phys = *pdpe & ~0xFFFULL;
            *pdpe = 0;
            pmm_free_page(pd_phys);

            if (table_is_empty(pdp->entries, 512)) {
                uint64_t pdp_phys = *pml4e & ~0xFFFULL;
                *pml4e = 0;
                pmm_free_page(pdp_phys);
            }
        }
    }

    return 0;
}

void vmm_switch_space(address_space_t *space) {
    asm volatile("mov %0, %%cr3" :: "r"(space->pml4_phys) : "memory");
}

address_space_t kernel_space_static;

address_space_t *vmm_create_kernel_space(void) {
    uint64_t pml4_phys = pmm_alloc_page();
    if (!pml4_phys) {
        serial_print("[VMM] FATAL: pmm_alloc_page = 0\n");
        return NULL;
    }

    pml4_t *new_pml4 = (pml4_t *)phys_to_virt(pml4_phys);

    uint64_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    pml4_t *limine_pml4 = (pml4_t *)phys_to_virt(cr3 & ~0xFFFULL);
    memcpy(new_pml4, limine_pml4, PAGE_SIZE);

    kernel_space_static.pml4t     = new_pml4;
    kernel_space_static.pml4_phys = pml4_phys;
    kernel_space_static.pid       = 0;
    kernel_space_static.ref_count = 1;

    return &kernel_space_static;
}

// Premières fonctions de ma forteresse
void enable_nxe(void) {
    uint32_t lo, hi;
    asm volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(0xC0000080));
    lo |= (1 << 11); // bit NXE
    asm volatile("wrmsr" :: "a"(lo), "d"(hi), "c"(0xC0000080));
}

void enable_wp(void) {
    uint64_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= (1ULL << 16);
    asm volatile("mov %0, %%cr0" :: "r"(cr0) : "memory");
}

void vmm_apply_nx(address_space_t *space) {
    // Passe 1 : remapper tout le kernel en RW d'abord
    for (uint64_t a = (uint64_t)&_text_start; a < (uint64_t)&_bss_end; a += PAGE_SIZE)
        vmm_map(space, a, kernel_virt_to_phys(a), PAGE_PRESENT | PAGE_RW);

    // Passe 2 : appliquer les permissions spécifiques
    // .text : exécutable, pas d'écriture, pas de NX
    for (uint64_t a = (uint64_t)&_text_start; a < (uint64_t)&_text_end; a += PAGE_SIZE)
        vmm_map(space, a, kernel_virt_to_phys(a), PAGE_PRESENT);

    // .rodata : lecture seule, NX
    for (uint64_t a = (uint64_t)&_rodata_start; a < (uint64_t)&_rodata_end; a += PAGE_SIZE)
        vmm_map(space, a, kernel_virt_to_phys(a), PAGE_PRESENT | PAGE_NX);

    // .data : lecture/écriture, NX (utilise _bss_start comme fin réelle)
    for (uint64_t a = (uint64_t)&_data_start; a < (uint64_t)&_bss_start; a += PAGE_SIZE)
        vmm_map(space, a, kernel_virt_to_phys(a), PAGE_PRESENT | PAGE_RW | PAGE_NX);

    // .bss : lecture/écriture, NX
    for (uint64_t a = (uint64_t)&_bss_start; a < (uint64_t)&_bss_end; a += PAGE_SIZE)
        vmm_map(space, a, kernel_virt_to_phys(a), PAGE_PRESENT | PAGE_RW | PAGE_NX);
}

int vmm_set_readonly(address_space_t *space, uint64_t virt_addr, uint64_t size) {
    uint64_t first_page = virt_addr & ~(PAGE_SIZE - 1);
    uint64_t last_page  = (virt_addr + size - 1) & ~(PAGE_SIZE - 1);

    for (uint64_t va = first_page; va <= last_page; va += PAGE_SIZE) {
        uint64_t pml4_idx = PML4_INDEX(va);
        uint64_t pdp_idx  = PDP_INDEX(va);
        uint64_t pd_idx   = PD_INDEX(va);
        uint64_t pt_idx   = PT_INDEX(va);

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

        *pte &= ~PAGE_RW;
        asm volatile("invlpg (%0)" :: "r"(va) : "memory");
    }

    return 0;
}

void vmm_map_framebuffer(address_space_t *space, Canvas *cv) {
    uint64_t fb_virt = (uint64_t)cv->address;
    uint64_t fb_phys = virt_to_phys(fb_virt);
    uint64_t fb_size = cv->pitch * cv->height;
    uint64_t pages   = (fb_size + PAGE_SIZE - 1) / PAGE_SIZE;

    for (uint64_t i = 0; i < pages; i++) {
        vmm_map(space,
                fb_virt + i * PAGE_SIZE,
                fb_phys + i * PAGE_SIZE,
                PAGE_PRESENT | PAGE_RW | PAGE_NX);
    }
}
