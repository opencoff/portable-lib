/*
 * t_pack.c: Pack/Unpack test harness.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "error.h"
#include "utils/utils.h"
#include "utils/pack.h"

static void test1(void);

int
main()
{
    test1();
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

