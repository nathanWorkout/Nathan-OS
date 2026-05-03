#ifndef STRING_H
#define STRING_H

#include <stdint.h>

int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, uint32_t n); 
int strlen(const char *s);

#endif
