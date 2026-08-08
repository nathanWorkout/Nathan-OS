#ifndef KMALLOC_H
#define KMALLOC_H

#include <stdint.h>

void *kmalloc(uint64_t size);
void  kfree(void *ptr);

#endif