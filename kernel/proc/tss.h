#ifndef TSS_H
#define TSS_H
#include <stdint.h>

static uint8_t irq1_stack[4096] __attribute__((aligned(16)));
static uint8_t exc_stack[8192]  __attribute__((aligned(16)));

typedef struct __attribute__((packed)) {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t io_map_base;
} tss_t;

void tss_init();
#endif