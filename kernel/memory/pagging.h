#ifndef PAGGING_H
#define PAGGING_H

#include <stdint.h>

void pagging_init();
uint32_t *pagging_create_page_directory();
uint32_t *pagging_create_table();
void pagging_map_page_table(uint32_t *pd, uint32_t *pt);
void pagging_enable(uint32_t *pd);

#endif
