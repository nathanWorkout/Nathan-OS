#include <stdint.h>

typedef struct __attribute__((packed)) {
    uint16_t limit_low;   // bits 0-15 of segment limit
    uint16_t base_low;    // bits 0-15 of base address
    uint8_t  base_middle; // bits 16-23 of base adddress
    uint8_t  access;      // access byte (present, DLP, type...)
    uint8_t  flags_limit; // upper 4 bits: flags, lower 4 bits : limit bits 16–19
    uint8_t  base_high;   // bits 24–31  of base address
} gdt_entry_t;

// GDTR : loaded with LGDT, tells the CPU where the GDT is
typedef struct __attribute__((packed)) {
    uint16_t limit; // size of GDT
    uint64_t base;  // address of GDT
} gdt_descriptor_t;

gdt_entry_t gdt[7]; // null, kernel code, kernel data, user code, user data, TSS low, TSS high
static gdt_descriptor_t gdtr;

static void gdt_set_entry(int i, uint64_t base, uint64_t limit, uint8_t access, uint8_t flags) {
    if (i >= 7) return;
    gdt[i].limit_low   = limit & 0xFFFF;
    gdt[i].base_low    = base & 0xFFFF;
    gdt[i].base_middle = (base >> 16) & 0xFF;
    gdt[i].access      = access;
    gdt[i].flags_limit = ((limit >> 16) & 0x0F) | (flags & 0xF0); // 2 niddle in 1 byte (requier by intel)
    gdt[i].base_high   = (base >> 24) & 0xFF;
}

// detailed documentation on my website
static inline void gdt_load(gdt_descriptor_t *gdtr) {
    __asm__ volatile (
        "lgdt (%0)\n"
        "lea 1f(%%rip), %%rax\n"
        "push $0x08\n"
        "push %%rax\n"
        "lretq\n"
        "1:\n"
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        : : "r"(gdtr) : "rax", "memory");
}

void gdt_init(void) {
    gdt_set_entry(0, 0, 0, 0, 0); // null descriptor
    gdt_set_entry(1, 0x00000000, 0x000FFFFF, 0x9A, 0xA0); // code kernel ring 0
    gdt_set_entry(2, 0x00000000, 0x000FFFFF, 0x92, 0x80); // data kernel ring 0
    gdt_set_entry(3, 0x00000000, 0x000FFFFF, 0xFA, 0xA0); // user code ring 3
    gdt_set_entry(4, 0x00000000, 0x000FFFFF, 0xF2, 0x80); // user data ring 3
    gdt_set_entry(5, 0, 0, 0x89, 0); // TSS low
    gdt_set_entry(6, 0, 0, 0, 0);    // TSS high

    gdtr.limit = (sizeof(gdt_entry_t) * 7) - 1;
    gdtr.base  = (uint64_t)&gdt;

    gdt_load(&gdtr);
}

void gdt_set_tss_entry(uint64_t base, uint64_t limit) {
    gdt_set_entry(5, base, limit, 0x89, 0x00);
    uint32_t *high = (uint32_t *)&gdt[6];
    high[0] = (base >> 32) & 0xFFFFFFFF;
    high[1] = 0;
    gdt_load(&gdtr);
}
