/*
 * Simple test harness for portable_readpass()
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "utils/readpass.h"


int
main()
{
    char *pw = 0;
    int r;

    r = portable_readpass(&pw, "passwd", "repeat passwd", 0);
    if (r == 0) { printf("Pass=|%s|\n", pw); free(pw); }

    r = portable_readpass(&pw, "passwd", 0, 0);
    if (r == 0) { printf("Pass2=|%s|\n", pw); free(pw); }
    return 0;
}

