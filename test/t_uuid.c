/*
 * Test harness for uuid_xxx()
 */
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "utils/uuid.h"


int
main(int argc, char **argv)
{
    int i;
    int n = 1;

    if (argc > 1) {
        n = atoi(argv[1]);
        if (n < 0) {
            fprintf(stderr, "Usage: %s [Integer]\n", argv[0]);
            exit(1);
        }
    }

    uint8_t b[16];
    char s[64];
    for (i = 0; i < n; i++) {
        int r = uuid_generate(b, sizeof b);
        assert(r == 16);

        r = uuid_tostring(s, sizeof s, b);
        assert(r == 37);
        printf("%s\n", s);
    }
    return 0;

}
