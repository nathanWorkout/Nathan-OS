#include <stdint.h>

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
