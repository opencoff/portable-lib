#include <stdio.h>
#include <stdint.h>

static inline int
__find_zbyte(uint64_t x)
{
    int n = 0;
    uint64_t y = (x & 0x7f7f7f7f7f7f7f7f) + 0x7f7f7f7f7f7f7f7f;

    y = ~(y | x | 0x7f7f7f7f7f7f7f7f);
    n = __builtin_clz(y) >> 7;
    return n;
}


/*
 * Find the first byte of hash 'a' in the occ word;
 * return the index.
 */
static inline int
__find_prefix(uint64_t occ, uint64_t a)
{
    printf("occ=%#lx, find=%#2.2x: ", occ, a);

    // extract the top byte
    a >>= 56;

    // fill the word with it
    a *= 0x0101010101010101;

    // XOR will zero out a similar byte in occ
    a ^= occ;

    // now, we use Henry Warren's trick to find the position of the
    // zerobyte
    a = __find_zbyte(a);
    printf("%d\n", a);
    return a < 8 ? a : -1;
}



int
main()
{
    int i = __find_prefix(0xdeadbeefbaadf00d, 0xad);
    return 0;
}
