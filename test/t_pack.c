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

struct testcase
{
    const char *fmt;
    const char *exphex;
    int nargs;
    void *args[16]; // max# of args.
};
typedef struct testcase testcase;

#define _1(fmt, exphex, v0) \
        {fmt, exphex, 1, { (void *)v0}}
#define _2(fmt, exphex, v0, v1) \
        {fmt, exphex, 2, { (void *)v0, (void *)v1}}
#define _3(fmt, exphex, v0, v1, v2) \
        {fmt, exphex, 3, { (void *)v0, (void *)v1, (void *)v2}}
#define _4(fmt, exphex, v0, v1, v2, v3) \
        {fmt, exphex, 4, { (void *)v0, (void *)v1, (void *)v2, (void *)v3}}


const uint8_t _4B[] = {0xa0, 0xa1, 0xa2, 0xa3 };
const testcase Tests[] = {
      _1("B", "03", 3)
    , _1("b", "fd", -3)
    , _1("4B", "a0a1a2a3", _4B)
    , _1("s", "ab06", 0x06ab)
    , _1(">S", "06ab", 0x06ab)

    , _1("I", "3412ab06", 0x06ab1234)
    , _1("> I", "06ab1234", 0x06ab1234)

    , _1("L", "78563412efbeadde", 0xdeadbeef12345678)

    , _2("B s", "feab06", 0xfe, 0x06ab)
    , _2("> B s", "fe06ab", 0xfe, 0x06ab)
    , _3("4B S I", "a0a1a2a334123412cdab", _4B, 0x1234, 0xabcd1234)

    , {0, 0, 0, {0}} // always keep at end
};


int
main()
{
    const testcase *t = &Tests[0];
    uint8_t exp[256];
    ssize_t nexp;

    uint8_t pk[256];
    ssize_t npk;

    void    *ux[16];
    uint8_t  bx;
    uint16_t sx;
    uint32_t ix;
    uint64_t lx;
    
    for (; t->fmt; ++t) {
        void * const *a = t->args;

        memset(exp, 0, sizeof exp);
        nexp = str2hex(exp, sizeof exp, t->exphex);
        assert(nexp > 0);

        memset(pk, 0, sizeof pk);

        /*
         * Pack tests
         */
        switch (t->nargs) {
            case 1:
                npk = Pack(pk, sizeof pk, t->fmt, a[0]);
                break;
            case 2:
                npk = Pack(pk, sizeof pk, t->fmt, a[0], a[1]);
                break;
            case 3:
                npk = Pack(pk, sizeof pk, t->fmt, a[0], a[1], a[2]);
                break;
            case 4:
                npk = Pack(pk, sizeof pk, t->fmt, a[0], a[1], a[2], a[3]);
                break;

            case 5:
                npk = Pack(pk, sizeof pk, t->fmt, a[0], a[1], a[2], a[3], a[4]);
                break;

            default:
                die("can't support more than 5 args..");
                break;
        }

        assert(npk == nexp);
        assert(0 == memcmp(exp, pk, npk));

        /*
         * Unpack tests
         */
    }
}
