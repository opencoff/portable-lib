/*
 * t_b64_c.c - Base-64 encoder/decoder self-test routine.
 * 
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include "error.h"

#include "utils/base64.h"

static void
test_run(uint8_t* inbuf, int inbufsz)
{
    char encbuf[1024];
    uint8_t decbuf[1024];
    int enclen = base64_encode_buf(encbuf, sizeof encbuf, inbuf, inbufsz);
    int declen = base64_decode_buf(decbuf, sizeof decbuf, encbuf, enclen);

    if (declen != inbufsz)
        error(1, 0, "Buf-%d: Length mismatch; exp %d saw %d", inbufsz, inbufsz, declen);

    if (0 != memcmp(decbuf, inbuf, inbufsz))
        error(1, 0, "Buf-%d: data mismatch", inbufsz);

    printf("Buf-%d => %d bytes: %s\n", inbufsz, enclen, encbuf);
}


int
main()
{
    int i = 0;
    uint8_t all[256];
    
    for (i = 0; i < 256; ++i)
        all[i] = i;

    for (i = 1; i <= 256; ++i)
        test_run(all, i);
    return 0;
}

