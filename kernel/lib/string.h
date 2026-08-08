#ifndef STRING_H
#define STRING_H
#include <stdint.h>
#include <stdint.h>

typedef unsigned long size_t;

int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, uint32_t n);
int strlen(const char *s);
void *memset(void *dst, int c, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
void strcat(char *dest, const char *src);

#endif
