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

static void test_b(void);
static void test_s(void);

int
main()
{
    test_b();
    test_s();
}

static void
test_b()
{
    uint8_t pk[8];
    ssize_t npk;
    ssize_t upk;
    int8_t  sb;
    uint8_t ub;

    memset(pk, 0, sizeof pk);
    npk = Pack(pk, sizeof pk, "B", 0x33);
    assert(npk == 1);
    assert(pk[0] == 0x33);

    upk = Unpack(pk, 1, "B", &ub);
    assert(upk == 1);
    assert(ub == 0x33);

    memset(pk, 0, sizeof pk);
    npk = Pack(pk, sizeof pk, "b", -3);
    assert(npk == 1);
    assert(pk[0] == 0xfd);

    upk = Unpack(pk, 1, "b", &sb);
    assert(upk == 1);
    assert(sb == -3);

    uint8_t b4[] = { 0xaa, 0xbb, 0xcc, 0xdd };
    uint8_t ub4[4] = { 0x11, 0x22, 0x33, 0x44 };

    memset(pk, 0, sizeof pk);
    npk = Pack(pk, sizeof pk, "4B", b4);
    assert(npk == 4);
    assert(pk[0] == 0xaa);
    assert(pk[1] == 0xbb);
    assert(pk[2] == 0xcc);
    assert(pk[3] == 0xdd);

    memset(ub4, 0, sizeof ub4);
    upk = Unpack(pk, npk, "4B", ub4);
    assert(upk == 4);
    assert(ub4[0] == 0xaa);
    assert(ub4[1] == 0xbb);
    assert(ub4[2] == 0xcc);
    assert(ub4[3] == 0xdd);

    memset(pk, 0, sizeof pk);
    npk = Pack(pk, sizeof pk, "B B B B", b4[0], b4[1], b4[2], b4[3]);
    assert(npk == 4);
    assert(pk[0] == 0xaa);
    assert(pk[1] == 0xbb);
    assert(pk[2] == 0xcc);
    assert(pk[3] == 0xdd);

    memset(ub4, 0, sizeof ub4);
    npk = Unpack(pk, npk, "B B B B", &ub4[0], &ub4[1], &ub4[2], &ub4[3]);
    assert(upk == 4);
    assert(ub4[0] == 0xaa);
    assert(ub4[1] == 0xbb);
    assert(ub4[2] == 0xcc);
    assert(ub4[3] == 0xdd);
}

static void
test_s()
{
    uint8_t pk[256];
    ssize_t npk;
    ssize_t upk;
    int16_t  sb;
    uint16_t ub;

    memset(pk, 0, sizeof pk);
    npk = Pack(pk, sizeof pk, "S", 0x33ff);
    assert(npk == 2);
    assert(pk[0] == 0xff);
    assert(pk[1] == 0x33);

    upk = Unpack(pk, npk, "S", &ub);
    assert(upk == 2);
    assert(ub == 0x33ff);

    memset(pk, 0, sizeof pk);
    npk = Pack(pk, sizeof pk, "s", (int16_t)-3);
    assert(npk == 2);
    assert(pk[0] == 0xfd);
    assert(pk[1] == 0xff);

    sb = 0;
    upk = Unpack(pk, npk, "s", &sb);
    assert(upk == 2);
    assert(sb == -3);

    uint16_t b4[] = { 0xaa11, 0xbb22, 0xcc33, 0xdd44 };
    uint16_t ub4[4];

    memset(pk, 0, sizeof pk);
    memset(ub4, 0, sizeof ub4);

    npk = Pack(pk, sizeof pk, "4S", b4);
    assert(npk == 8);
    assert(pk[0] == 0x11);
    assert(pk[1] == 0xaa);
    assert(pk[2] == 0x22);
    assert(pk[3] == 0xbb);
    assert(pk[4] == 0x33);
    assert(pk[5] == 0xcc);
    assert(pk[6] == 0x44);
    assert(pk[7] == 0xdd);

    upk = Unpack(pk, npk, "4S", ub4);
    assert(upk == 8);
    assert(ub4[0] == 0xaa11);
    assert(ub4[1] == 0xbb22);
    assert(ub4[2] == 0xcc33);
    assert(ub4[3] == 0xdd44);

    memset(pk, 0, sizeof pk);
    memset(ub4, 0, sizeof ub4);

    npk = Pack(pk, sizeof pk, "S S S S", b4[0], b4[1], b4[2], b4[3]);
    assert(npk == 8);
    assert(pk[0] == 0x11);
    assert(pk[1] == 0xaa);
    assert(pk[2] == 0x22);
    assert(pk[3] == 0xbb);
    assert(pk[4] == 0x33);
    assert(pk[5] == 0xcc);
    assert(pk[6] == 0x44);
    assert(pk[7] == 0xdd);

    npk = Unpack(pk, npk, "S S S S", &ub4[0], &ub4[1], &ub4[2], &ub4[3]);
    assert(upk == 8);
    assert(ub4[0] == 0xaa11);
    assert(ub4[1] == 0xbb22);
    assert(ub4[2] == 0xcc33);
    assert(ub4[3] == 0xdd44);
}

#if 0
static void
test_i()
{
    uint8_t pk[8];
    ssize_t npk;
    ssize_t upk;
    int8_t  sb;
    uint8_t ub;

    npk = Pack(pk, sizeof pk, "B", 0x33);
    assert(npk == 1);
    assert(pk[0] = 0x33);

    upk = Unpack(pk, 1, "B", &ub);
    assert(upk == 1);
    assert(ub == 0x33);

    npk = Pack(pk, sizeof pk, "b", -3);
    assert(npk == 1);
    assert(pk[0] = 0xfd);

    upk = Unpack(pk, 1, "B", &sb);
    assert(upk == 1);
    assert(sb == -3);

    uint8_t b4[] = { 0xaa, 0xbb, 0xcc, 0xdd };
    uint8_t ub4[4] = { 0x11, 0x22, 0x33, 0x44 };

    memset(pk, 0, sizeof pk);
    npk = Pack(pk, sizeof pk, "4B", b4);
    assert(npk == 4);
    assert(pk[0] == 0xaa);
    assert(pk[1] == 0xbb);
    assert(pk[2] == 0xcc);
    assert(pk[3] == 0xdd);

    upk = Unpack(pk, npk, "4B", ub4);
    assert(upk == 4);
    assert(ub4[0] == 0xaa);
    assert(ub4[1] == 0xbb);
    assert(ub4[2] == 0xcc);
    assert(ub4[3] == 0xdd);

    memset(pk, 0, sizeof pk);
    npk = Pack(pk, sizeof pk, "B B B B", b4[0], b4[1], b4[2], b4[3]);
    assert(npk == 4);
    assert(pk[0] == 0xaa);
    assert(pk[1] == 0xbb);
    assert(pk[2] == 0xcc);
    assert(pk[3] == 0xdd);

    npk = Unpack(pk, npk, "B B B B", &b4[0], &b4[1], &b4[2], &b4[3]);
    assert(upk == 4);
    assert(ub4[0] == 0xaa);
    assert(ub4[1] == 0xbb);
    assert(ub4[2] == 0xcc);
    assert(ub4[3] == 0xdd);
}

static void
test_l()
{
    uint8_t pk[8];
    ssize_t npk;
    ssize_t upk;
    int8_t  sb;
    uint8_t ub;

    npk = Pack(pk, sizeof pk, "B", 0x33);
    assert(npk == 1);
    assert(pk[0] = 0x33);

    upk = Unpack(pk, 1, "B", &ub);
    assert(upk == 1);
    assert(ub == 0x33);

    npk = Pack(pk, sizeof pk, "b", -3);
    assert(npk == 1);
    assert(pk[0] = 0xfd);

    upk = Unpack(pk, 1, "B", &sb);
    assert(upk == 1);
    assert(sb == -3);

    uint8_t b4[] = { 0xaa, 0xbb, 0xcc, 0xdd };
    uint8_t ub4[4] = { 0x11, 0x22, 0x33, 0x44 };

    memset(pk, 0, sizeof pk);
    npk = Pack(pk, sizeof pk, "4B", b4);
    assert(npk == 4);
    assert(pk[0] == 0xaa);
    assert(pk[1] == 0xbb);
    assert(pk[2] == 0xcc);
    assert(pk[3] == 0xdd);

    upk = Unpack(pk, npk, "4B", ub4);
    assert(upk == 4);
    assert(ub4[0] == 0xaa);
    assert(ub4[1] == 0xbb);
    assert(ub4[2] == 0xcc);
    assert(ub4[3] == 0xdd);

    memset(pk, 0, sizeof pk);
    npk = Pack(pk, sizeof pk, "B B B B", b4[0], b4[1], b4[2], b4[3]);
    assert(npk == 4);
    assert(pk[0] == 0xaa);
    assert(pk[1] == 0xbb);
    assert(pk[2] == 0xcc);
    assert(pk[3] == 0xdd);

    npk = Unpack(pk, npk, "B B B B", &b4[0], &b4[1], &b4[2], &b4[3]);
    assert(upk == 4);
    assert(ub4[0] == 0xaa);
    assert(ub4[1] == 0xbb);
    assert(ub4[2] == 0xcc);
    assert(ub4[3] == 0xdd);
}
#endif
