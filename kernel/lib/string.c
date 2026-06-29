#include <stdint.h>

typedef unsigned long size_t;

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }

    return (unsigned char)*s1 - (unsigned char)*s2;
}

int strncmp(const char *s1, const char *s2, uint32_t n) {
  unsigned char u1, u2;

  while (n-- > 0) {
      u1 = (unsigned char) *s1++;
      u2 = (unsigned char) *s2++;
      if (u1 != u2)
	      return u1 - u2;
      if (u1 == '\0')
	      return 0;
      }
  return 0;
}

int strlen(const char *s) {
    int i = 0;
    while (s[i]) i++;
    return i;
}

void *memset(void *dst, int c, size_t n) {
    unsigned char *p = dst;
    while (n--) {
        *p++ = (unsigned char)c;
    }

    return dst;
}

void *memcpy(void *dst, const void *src, size_t n) {
    unsigned char *d = dst;
    const unsigned char *s = src;
    while (n--) {
        *d++ = *s++;
    }

    return dst;
}
