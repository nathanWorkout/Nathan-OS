#pragma once
#include <stdint.h>
#include <stddef.h>
#include "limine.h"

extern volatile struct limine_memmap_request         memmap_request;
extern volatile struct limine_hhdm_request           hhdm_request;
extern volatile struct limine_kernel_address_request kernel_address_request;
extern volatile struct limine_module_request         module_request;

static inline uint64_t phys_to_virt(uint64_t phys) {
    if (hhdm_request.response == NULL) return phys;
    return phys + hhdm_request.response->offset;
}

static inline uint64_t virt_to_phys(uint64_t virt) {
    if (hhdm_request.response == NULL) return virt;
    return virt - hhdm_request.response->offset;
}
