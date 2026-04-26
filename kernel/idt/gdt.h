#ifndef GDT_H
#define GDT_H

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

void gdt_init(void);
void gdt_set_tss_entry(uint32_t base, uint32_t limit);

#endif
