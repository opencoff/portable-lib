/*
 * Test harness for frand()
 */


#include <stdio.h>


extern double frand(void);

int
main()
{
    int i;

    for (i = 0; i < 100; i++) {
        printf("%6.5f\n", frand());
    }
    return 0;
}

