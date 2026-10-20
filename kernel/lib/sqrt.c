#include <stdint.h>

/*
 * Newton-Raphson formula to approximate a square root iteratively :
 * (I DON'T HAVE FPU AND FLOAT NUMBER ARE DISABLE)
 *
 * Example for sqrt(4) :
 * 4 * 1 000 000 = 4 000 000 (to get decimals at the end without having a flaot number)
 *
 * Technical notions : tmp >>= 2 is like divide per 4 (beceause 2² = 4)
 * Example with 8 : 1000 >> 2 = 0010 : 8 / 4 = 2
 *
 * Same with tmp <<= 1 : is like multiplie per 2
 * Example with 1 (beceause by default : int64_t y = 1;)
 * 0001 << 1 = 0010 = 2
 *
 * We do that beceause is we do while (y * y < n) y++; it's too  slow (3000 turn into the loop against max 32 here)
 *
 * So in this case (for sqrt(4)), tmp >>= 2 is like tmp / 4 and y <<= 1 is like y = y * 2
 *
 * Mathematical property : sqrt(n / 4) = sqrt(n) / 2
 * Demonstration :
 * sqrt = y * y
 * y * y = n / 4
 *       = (sqrt(n) / 2) * (sqrt(n) / 2)
 *       = (sqrt(n) * (sqrt(n) / (2 * 2)))
 *       = n / 4
 *
 * With a n = 4 :
 * y * y = n / 4
 *       = (sqrt(4) / 2 * sqrt(4) / 2)
 *       = (2 / 2) * (2 / 2)
 *       = 1 * 1
 *       = 1
 *
 * and n / 4 = 4 / 4 = 1
 *
 * In this case (n = 4 000 000) :
 * tmp = 4 000 000    y = 1
 * tmp = 1 000 000    y = 2
 * tmp = 250 000      y = 4
 * tmp = 62 500       y = 8
 * tmp = 15 625       y = 16
 * tmp = 3 906        y = 32
 * tmp = 976          y = 64
 * tmp = 244          y = 128
 * tmp = 61           y = 256
 * tmp = 15           y = 512
 * tmp = 3            y = 1024
 * tmp = 0            y = 2048 // start point
 *
 *
 * It's used to pre-calculate a starting number for the following calculations in order to avoid doing too many loops (here : 2048)
 *
 * To continue, we want to found y such as y * y = n
 * f(y) = y² - n
 * We search f(y) = 0 like y² = n like y = sqrt(n)
 *
 * y_next = y - f(y) / f'(y) // Newton's formula
 *
 * f'(y) = 2y
 * y_next = y - f(y) / f'(y)
 *      = y - (y² - n) / 2y
 *      = y - y²/2y + n/2y
 *      = y - y/2 + n/2y
 *      = y/2 + n/2y
 *      = (y + n/y) / 2
 *
 * In this case : (n = 4 000 000 and y = 2048)
 *
 * turn 1 of loop :
 * y_next = (2048 + 4000000/2048) / 2
 *        = (2048 + 1953) / 2
 *        = 4001 / 2
 *        = 2000
 *
 * y_next = (2000 + 4000000/2000) / 2
 *        = (2000 + 2000) / 2
 *        = 4000 / 2
 *         = 2000 y_next >=y -> break
 *
 * y = 2000 for n = 4 000 000
 * the more i is increased, the more the number of turns increases, the more n can make sqrt greater
 */


// WHEN YOU USE THIS FUNCTION REMEMBER TO DIVIDE BY 1000 IF YOU DON'T WANT DECIMALS
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
