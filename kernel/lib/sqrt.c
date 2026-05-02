#include <stdint.h>

int64_t sqrt(int64_t x) {
    if (x <= 0) return 0;
    int64_t n = x * 1000000LL;

    int64_t y = 1;
    int64_t tmp = n;
    while (tmp > 0) { tmp >>= 2; y <<= 1; } 
    

    for (int i = 0; i < 32; i++) {
        int64_t y_next = (y + n / y) / 2;
        if (y_next >= y) break;
        y = y_next;
    }
    return y;
}