#include <stdint.h>

typedef struct __attribute__((packed)) {
    uint16_t limit_low;    
    uint16_t base_low;     
    uint8_t  base_middle;  
    uint8_t  access;       
    uint8_t  flags_limit;  
    uint8_t  base_high;    
} gdt_entry_t;

typedef struct __attribute__((packed)) {
    uint16_t limit;  
    uint32_t base;   
} gdt_descriptor_t;

static gdt_entry_t gdt[3];
static gdt_descriptor_t gdtr;

static void gdt_set_entry(int i, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    gdt[i].limit_low   = limit & 0xFFFF;
    gdt[i].base_low    = base & 0xFFFF;
    gdt[i].base_middle = (base >> 16) & 0xFF;
    gdt[i].access      = access;
    gdt[i].flags_limit = ((limit >> 16) & 0x0F) | (flags & 0xF0);
    gdt[i].base_high   = (base >> 24) & 0xFF;
}

static inline void gdt_load(gdt_descriptor_t *gdtr) {
    __asm__ volatile ("lgdt (%0)" : : "r"(gdtr));
}

void gdt_init(void) {
    gdt_set_entry(0, 0, 0, 0, 0);

    gdt_set_entry(1, 0x00000000, 0x000FFFFF, 0x9A, 0xCF);

    gdt_set_entry(2, 0x00000000, 0x000FFFFF, 0x92, 0xCF);

    gdtr.limit = (sizeof(gdt_entry_t) * 3) - 1;
    gdtr.base  = (uint32_t)&gdt;

    gdt_load(&gdtr);
}
