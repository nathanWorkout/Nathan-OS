#include <stdint.h>

typedef unsigned long size_t;

// Compare two strings, return 0 if equal, negative if s1 < s2, positive if s1 > s2
int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }

    return (unsigned char)*s1 - (unsigned char)*s2;
}

// Compare at most n characters of two strings
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

// Return the length of a string, not counting the null terminator
int strlen(const char *s) {
    int i = 0;
    while (s[i]) i++;
    return i;
}

// Fill n bytes of memory at dst with byte c
void *memset(void *dst, int c, size_t n) {
    unsigned char *p = dst;
    while (n--) {
        *p++ = (unsigned char)c;
    }

    return dst;
}

// Copy n bytes from src to dst
void *memcpy(void *dst, const void *src, size_t n) {
    unsigned char *d = dst;
    const unsigned char *s = src;
    while (n--) {
        *d++ = *s++;
    }

    return dst;
}

// Compare n bytes of two memory regions, return 0 if equal
int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *a = s1;
    const unsigned char *b = s2;
    while (n--) {
        if (*a != *b) return *a - *b;
        a++;
        b++;
    }
    return 0;
}

//  Append src to the end of dest
void strcat(char *dest, const char *src) {
    int i = strlen(dest);
    int j = 0;
    while (src[j] != '\0') {
        dest[i++] = src[j++];
    }
    dest[i] = '\0';
}
