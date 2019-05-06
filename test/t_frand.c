/*
 * Test harness for frand()
 */
#include <stdio.h>
#include <stdlib.h>


extern double frand(void);

int
main(int argc, const char *argv[])
{
    int n = 10;
    int i;

    if (argc >= 2) {
        n = atoi(argv[1]);
        if (n < 0) {
            fprintf(stderr, "invalid count %s (not a number?)\n", argv[1]);
            return 1;
        }
    }

    for (i = 0; i < n; i++) {
        printf("%6.11f\n", frand());
    }
    return 0;
}

