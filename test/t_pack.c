/*
 * t_pack.c: Pack/Unpack test harness.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "error.h"
#include "utils/utils.h"
#include "utils/pack.h"

static void test1(void);
static void testmulti(void);
static void testvararg(void);
static void teststr(void);
static void testerr(void);
static void hexdump(const char* prefix, const void * buf, size_t bufsiz);


#define xassert(a,fmt,...) do {\
                                if (!(a)) { \
                                    fprintf(stderr, fmt "\n", __VA_ARGS__);\
                                    hexdump(#a, pbuf, psz);\
                                }\
                                assert(a);\
                            } while (0)

int
main()
{
    test1();
    testmulti();
    testvararg();
    teststr();
    testerr();
    return 0;
}

// Test multiple arguments of various type
static void
testmulti()
{
    uint8_t pbuf[256];
    ssize_t psz;
    uint8_t  bva[2] = { 0x33, 0x55 };
    uint32_t iva[2] = { 0x12345678, 0xabcd9988 };
    uint64_t lv     = 0xdeadbeefbaadf00d;

    memset(pbuf, 0, sizeof pbuf);
    psz = Pack(pbuf, sizeof pbuf, ">  2B   2I   \tL", bva, iva, lv);
    assert(psz == 18);

    uint8_t *x = pbuf;
    assert(*x++ == 0x33);
    assert(*x++ == 0x55);

    assert(*x++ == 0x12);
    assert(*x++ == 0x34);
    assert(*x++ == 0x56);
    assert(*x++ == 0x78);

    assert(*x++ == 0xab);
    assert(*x++ == 0xcd);
    assert(*x++ == 0x99);
    assert(*x++ == 0x88);

    assert(*x++ == 0xde);
    assert(*x++ == 0xad);
    assert(*x++ == 0xbe);
    assert(*x++ == 0xef);
    assert(*x++ == 0xba);
    assert(*x++ == 0xad);
    assert(*x++ == 0xf0);
    assert(*x++ == 0x0d);

    // now unpack.
    memset(bva, 0, sizeof bva);
    memset(iva, 0, sizeof iva);
    lv = 0;
    psz = Unpack(pbuf, psz, "> 2B 2I L", bva, iva, &lv);
    assert(psz == 18);
    assert(bva[0] == 0x33);
    assert(bva[1] == 0x55);
    assert(iva[0] == 0x12345678);
    assert(iva[1] == 0xabcd9988);
    assert(lv     == 0xdeadbeefbaadf00d);
}

// test var-arg for simple types.
// XXX We only do _one_ type. No need for anything else.
static void
testvararg()
{
    uint8_t pbuf[256];
    ssize_t psz;
    uint8_t  bva[4] = { 0x33, 0x55, 0x77, 0x99 };
    uint8_t *uva    = 0;
    uint32_t nargs  = 0;

    memset(pbuf, 0, sizeof pbuf);
    psz = Pack(pbuf, sizeof pbuf, "> * B", 4, bva);
    assert(psz == 8);

    uint8_t *x = pbuf;
    assert(*x++ == 0x00);
    assert(*x++ == 0x00);
    assert(*x++ == 0x00);
    assert(*x++ == 0x04);

    assert(*x++ == 0x33);
    assert(*x++ == 0x55);
    assert(*x++ == 0x77);
    assert(*x++ == 0x99);


    memset(bva, 0, sizeof bva);
    psz = Unpack(pbuf, psz, "> * B", &nargs, &uva);
    assert(psz == 8);
    assert(nargs == 4);

    x = uva;
    assert(*x++ == 0x33);
    assert(*x++ == 0x55);
    assert(*x++ == 0x77);
    assert(*x++ == 0x99);
    DEL(uva);
}

// Test strings
static void
teststr()
{
    const char *ps     = "hello";
    const size_t pslen = strlen(ps);
    uint8_t pbuf[256];
    ssize_t psz;
    char *us      = 0; // unpacked string
    uint32_t slen = 0; // unpacked string length
    uint32_t vv   = 0;


    // Invalid.
    psz = Pack(pbuf, sizeof pbuf, "* 2Z", 4, ps);
    xassert(psz < 0, "psz=%ld", psz);

    psz = Pack(pbuf, sizeof pbuf, "> Z", ps);
    assert((size_t)psz == (1 + pslen));
    assert(0 == memcmp(pbuf, ps, pslen));

    psz = Unpack(pbuf, psz, "> Z", &us);
    xassert((size_t)psz == (1 + pslen), "psz=%ld", psz);
    assert(us);
    assert(strlen(us) == pslen);
    assert(0 == strcmp(us, ps));
    free(us);

    psz = Pack(pbuf, sizeof pbuf, "3Z", ps);
    xassert(psz == 3, "psz=%ld", psz);

    psz = Unpack(pbuf, psz, "3Z", &us);
    assert(psz == 3);
    assert(us);
    xassert(strlen(us) == 3, "us=|%s|", us);
    free(us);

    psz = Pack(pbuf, sizeof pbuf, "* Z", 4, ps);
    xassert(psz == 8, "psz=%ld", psz);

    psz = Unpack(pbuf, psz, "* Z", &slen, &us);
    xassert(psz == 8, "psz=%ld", psz);
    assert(us);
    xassert(slen == 4, "slen=%u", slen);
    xassert(strlen(us) == 4, "us=|%s|", us);
    assert(strcmp("hell", us) == 0);
    free(us);

    psz = Pack(pbuf, sizeof pbuf, "* Z I", 10, ps, 0xbaadf00d);
    assert(psz == 18);

    psz = Unpack(pbuf, psz, "* Z I", &slen, &us, &vv);
    assert(psz == 18);
    assert(slen == 10);
    assert(us);
    assert(strlen(us) == 5);
    assert(vv == 0xbaadf00d);
    free(us);

    psz = Pack(pbuf, sizeof pbuf, "8Z 8x I", ps, 0xdeadbeef);
    assert(psz == 20);

    psz = Unpack(pbuf, psz, "8z 8x I", &us, &vv);
    assert(psz == 20);
    assert(us);
    assert(strlen(us) == 5);
    assert(strcmp(ps, us) == 0);
    assert(vv == 0xdeadbeef);
    free(us);
}


// test error paths
static void
testerr()
{
    uint8_t pbuf[4];
    ssize_t n;

    n = Pack(pbuf, sizeof pbuf, "> Z I I", "hello", 0xf0001000, 0xd000100c);
    assert(n < 0);
    assert(n == -ENOSPC);
}

// test exactly one value 'val' of type 'typ' with format 'fmt'.
#define xtest1(typ, psz, fmt, val, ...) \
                do { \
                    const uint8_t pval[] = { \
                        __VA_ARGS__  \
                    };\
                    int i;\
                    uint8_t pbuf[psz];\
                    const typ   pv = (typ)val;\
                    typ   uv;\
                    ssize_t sz; \
                    memset(pbuf, 0, sizeof pbuf);\
                    memset(&uv, 0, sizeof uv);\
                    sz = Pack(&pbuf[0], sizeof pbuf, fmt, pv);\
                    assert(sz == psz);\
                    for(i=0; i<psz; i++) {\
                        assert(pbuf[i] == pval[i]);\
                    }\
                    sz = Unpack(&pbuf[0], sz, fmt, &uv);\
                    assert(sz == psz);\
                    assert(uv == pv);\
                }while (0)

// test 4 items of a single type 'typ'
#define xtest4(typ, psz, fmt, val, ...) \
                do { \
                    const uint8_t pval[] = { \
                        __VA_ARGS__  \
                    };\
                    const ssize_t expsz = psz * 4; \
                    const typ pv     = (typ)val;\
                    int i;\
                    uint8_t pbuf[expsz];\
                    uint8_t *x = &pbuf[0];\
                    const typ pva[4] = { pv, pv, pv, pv };\
                    typ uva[4];\
                    ssize_t sz; \
                    memset(pbuf, 0, sizeof pbuf);\
                    memset(&uva, 0, sizeof uva);\
                    sz = Pack(&pbuf[0], sizeof pbuf, fmt, &pva[0]);\
                    assert(sz == expsz);\
                    for(i=0; i < 4; i++) {\
                        int j;\
                        for(j=0; j<psz; j++, x++) {\
                            assert(*x == pval[j]);\
                        }\
                    }\
                    sz = Unpack(&pbuf[0], sz, fmt, &uva);\
                    assert(expsz == sz);\
                    for(i=0; i < 4;i++) {\
                        assert(uva[i] == pva[i]);\
                    }\
                }while (0)

    
static void
test1(void)
{
    xtest1(uint8_t, 1, "B", 0x33, 0x33);
    xtest1( int8_t, 1, "b", -1,   0xff);

    xtest1(uint16_t, 2, "S",  0x1234,   0x34, 0x12);
    xtest1(uint16_t, 2, ">S", 0x1234,   0x12, 0x34);
    xtest1( int16_t, 2, "s",  -3,       0xfd, 0xff);
    xtest1( int16_t, 2, ">s", -3,       0xff, 0xfd);

    xtest1(uint32_t, 4, "I",  0xabcd1234,   0x34, 0x12, 0xcd, 0xab);
    xtest1(uint32_t, 4, ">I", 0xabcd1234,   0xab, 0xcd, 0x12, 0x34);
    xtest1( int32_t, 4, "i",  -3,       0xfd, 0xff, 0xff, 0xff);
    xtest1( int32_t, 4, ">i", -3,       0xff, 0xff, 0xff, 0xfd);

    xtest1(uint64_t, 8, "L",  0xdeadbeefabcd1234,   0x34, 0x12, 0xcd, 0xab, 0xef, 0xbe, 0xad, 0xde);
    xtest1(uint64_t, 8, ">L", 0xdeadbeefabcd1234,   0xde, 0xad, 0xbe, 0xef, 0xab, 0xcd, 0x12, 0x34);
    xtest1( int64_t, 8, "l",  -3,       0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff);
    xtest1( int64_t, 8, ">l", -3,       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfd);

    // test 4 item vect

    xtest4(uint8_t, 1, "4B", 0x33, 0x33);
    xtest4( int8_t, 1, "4b", -1,   0xff);

    xtest4(uint16_t, 2, "4S",  0x1234,   0x34, 0x12);
    xtest4(uint16_t, 2, ">4S", 0x1234,   0x12, 0x34);
    xtest4( int16_t, 2, "4s",  -3,       0xfd, 0xff);
    xtest4( int16_t, 2, ">4s", -3,       0xff, 0xfd);

    xtest4(uint32_t, 4, "4I",  0xabcd1234,   0x34, 0x12, 0xcd, 0xab);
    xtest4(uint32_t, 4, ">4I", 0xabcd1234,   0xab, 0xcd, 0x12, 0x34);
    xtest4( int32_t, 4, "4i",  -3,       0xfd, 0xff, 0xff, 0xff);
    xtest4( int32_t, 4, ">4i", -3,       0xff, 0xff, 0xff, 0xfd);

    xtest4(uint64_t, 8, "4L",  0xdeadbeefabcd1234,   0x34, 0x12, 0xcd, 0xab, 0xef, 0xbe, 0xad, 0xde);
    xtest4(uint64_t, 8, ">4L", 0xdeadbeefabcd1234,   0xde, 0xad, 0xbe, 0xef, 0xab, 0xcd, 0x12, 0x34);
    xtest4( int64_t, 8, "4l",  -3,       0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff);
    xtest4( int64_t, 8, ">4l", -3,       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfd);
}

static void
hexdump(const char* prefix, const void * buf, size_t bufsiz)
{
    const char hex[]   = "0123456789abcdef";
    const uint8_t * p  = (const uint8_t*)buf;
    size_t rem         = bufsiz;
    char bb[128];

    printf("%s: %p %zu bytes\n", prefix, buf, bufsiz);

    while (rem > 0)
    {
        char * ptr   = &bb[0];
        char * ascii = ptr + ((8 * 2 + 7) * 2) + 4;
        size_t n = rem > 16 ? 16 : rem;
        const uint8_t * pe = p + n;
        int z = 0;

        rem -= n;
        memset(bb, ' ', sizeof bb);

        for (z = 0; p < pe; ++p, ++z)
        {
            unsigned char c = *p;
            *ptr++ = hex[c >> 4];
            *ptr++ = hex[c & 0xf];
            *ptr++ = ' ';
            if (isprint(c))
                *ascii++ = c;
            else
                *ascii++ = '.';

            if (z == 7)
            {
                *ptr++   = ' ';
                *ascii++ = ' ';
            }
        }
        *ascii = 0;

        printf("%p %s\n", p, bb);
    }
}
