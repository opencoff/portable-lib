/*
 * Example use of duffs device
 * Apr 1997 Sudhi Herle <sw at herle.net>
 */
#include <stdio.h>
#include <stdlib.h>

int
duffs(unsigned int v)
{
    /*
     * Duffs device unrolled 4 times.
     */
#define UNROLL  4
    unsigned int n = v / UNROLL;
    unsigned int r = v % UNROLL;
    int i = 0;

    if (n > 0)
    {
        i += (UNROLL * n);

        /*
         * We need as many case statements as UNROLL
         */

        switch (n & 3) {
            case 0: do {
                n--;

            case 3:
                n--;

            case 2:
                n--;

            case 1:
                n--;
            } while (n > 0);
        }
    }

    while (r > 0)
    {
        i++;
        r--;
    }
    //printf("%d\n", i);

    return i;
}

int
main()
{
    unsigned int i;

    for (i = 0; i < 15; i++)
    {
        int j = duffs(i);

        if (j != i)
        {
            fprintf(stderr, "%d: %d != %d\n", i, i, j);
            exit(1);
        }
    }
    return 0;
}
