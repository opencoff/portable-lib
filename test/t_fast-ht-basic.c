/*
 * Sanity Test for cache-friendly, super-fast hash table.
 *
 * (c) 2022 Sudhi Herle <sudhi-at-herle.net>
 */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "error.h"
#include "utils/typeutils.h"
#include "utils/fast-ht.h"
#include "fast/vect.h"
#include "utils/utils.h"


struct kv {
    uint64_t key;
    uint64_t val;
};
typedef struct kv kv;

const kv Kvpairs[] = {
      {0x3154943e5c03bd69, 64}
	, {0xa896836ae76aa1e2, 63}
	, {0xaaebb342645e58fd, 62}
	, {0xb1113fd30ce5eb95, 61}
	, {0x190a817ef5069cf3, 60}
	, {0x266d2f4a8b25041a, 59}
	, {0x81b42dd50d98665f, 58}
	, {0x4c0d9e043668d4cb, 57}
	, {0x4741f412cf384a5a, 56}
	, {0x77c9e0830c855dbc, 55}
	, {0x35b6aa1404b0d6d0, 54}
	, {0x3ec772e4ab7a2743, 53}
	, {0xb1704a17e12f29bd, 52}
	, {0x1cb85b9d199600a1, 51}
	, {0xf47d5b8fdaf86814, 50}
	, {0xdf043bc824eee0ba, 49}
	, {0x2eb0330772cc8c87, 48}
	, {0x0382a8473ef2e137, 47}
	, {0x33cb9ddcf97a1045, 46}
	, {0xa6f78489f190e58e, 45}
	, {0x8a17037bca7b79a5, 44}
	, {0xf7b766913c90855e, 43}
	, {0xf0bd201b33ce64ce, 42}
	, {0xf62be24fbb9d88ff, 41}
	, {0x9048358012a56494, 40}
	, {0x526c9cb615334fda, 39}
	, {0xf0405f425a9f156f, 38}
	, {0xbf1b3c6b76eed630, 37}
	, {0x224b67b3c87912ac, 36}
	, {0xc7c85b4ba3a942c5, 35}
	, {0xefa3a397b65ad2c2, 34}
	, {0x6a0eba0f72f3323f, 33}
	, {0xd740797ffe17837c, 32}
	, {0x7181fb8ae860c6c7, 31}
	, {0x70f0a7015190e884, 30}
	, {0x46b62db085f8a711, 29}
	, {0xd0cc9b4a64e1c414, 28}
	, {0x49f78aac19e2d093, 27}
	, {0x3734686a1c555888, 26}
	, {0xc9202c731d659738, 25}
	, {0x3cc4865c0206b135, 24}
	, {0x83396186144ab9bd, 23}
	, {0x8ba81e88c653c7bf, 22}
	, {0xebf32f352b7654fb, 21}
	, {0xcba2ce9e2b327782, 20}
	, {0x1717d07fdd637c0a, 19}
	, {0xed4354469bb0c75c, 18}
	, {0xd1d7d360f47cd410, 17}
	, {0xda2c70bcf0806187, 16}
	, {0xae978ed49f0f96d2, 15}
	, {0x8611bd0bafb2932a, 14}
	, {0xa61a7dd88d6371d8, 13}
	, {0xf359ae035e0c5570, 12}
	, {0x19e4656c7c8ebc92, 11}
	, {0xd3d7cd51e30da9b7, 10}
	, {0x8940abff17c615dd, 9}
	, {0x6683f42c71431eea, 8}
	, {0xa44e191e5f867a82, 7}
	, {0xe84142e6d970bdc7, 6}
	, {0xe3622b32607c401e, 5}
	, {0xc95ac2dffa31e498, 4}
	, {0x24f6a22a0ef32b89, 3}
	, {0xa7cab94554af9a63, 2}
	, {0x896ac775b9d08475, 1}
};

static void
dump(const char *str, size_t n)
{
    fputs(str, stdout);
}

static uint64_t
rand64()
{
    union z {
        uint64_t v;
        uint8_t  b[8];
    } u;


    u.v = 0;
    arc4random_buf(u.b, sizeof u.b);
    return u.v;
}


static void
basic_tests()
{
    ht  _h;
    ht *h = &_h;

    ht_init(h, 3, 85);

    const kv *p = &Kvpairs[0],
             *e = p + ARRAY_SIZE(Kvpairs);


    for (; p < e; p++) {
        void *r = ht_probe(h, p->key, (void *)p->val);

        // Every one of the keys _are be unique. Thus, we must not
        // find dups.
        assert(!r);
    }

   // ht_dump(h, "after insert", dump);

    // now search
    for (p = &Kvpairs[0]; p < e; p++) {
        void *ret = 0;
        int r = ht_find(h, p->key, &ret);

        assert(r);
        assert(ret == (void *)p->val);
    }

    // now delete some of them.
    for (p = &Kvpairs[0]; p < e; p++) {
        void *ret = 0;

        // delete odd valued keys
        if (p->key & 1) {
            int r = ht_remove(h, p->key, &ret);

            assert(r);
            assert(ret == (void *)p->val);
        }
    }

    // now search
    for (p = &Kvpairs[0]; p < e; p++) {
        void *ret = 0;

        // we shouldn't find these keys
        if (p->key & 1) {
            int r = ht_find(h, p->key, &ret);

            assert(!r);
        } else {
            int r = ht_find(h, p->key, &ret);

            assert(r);
            assert(ret == (void *)p->val);
        }
    }
    // cleanup
    ht_fini(h);
}


VECT_TYPEDEF(kvv, kv);

static void
rand_tests()
{
    ht  _h;
    ht *h = &_h;
    kvv  v;
    kv *p;

#define NELEM    128*128

    // generate N random numbers
    VECT_INIT(&v, NELEM);
    for (int i = 0; i < NELEM; i++) {
        kv w = {
            .key = rand64(),
            .val = (uint64_t)i,
        };

        VECT_PUSH_BACK(&v, w);
    }

    ht_init(h, VECT_LEN(&v), 85);

    uint64_t cyi = 0,
             cyf = 0,
             cyd = 0,
             cyx = 0;

    uint64_t ti  = 0,
             tf  = 0,
             td  = 0,
             tx  = 0;
    uint64_t c0, t0;

    VECT_FOR_EACH(&v, p) {
        t0 = timenow();
        c0 = sys_cpu_timestamp();
        void *r = ht_probe(h, p->key, (void *)p->val);
        ti  += timenow() - t0;
        cyi += sys_cpu_timestamp() - c0;

        // Every one of the keys _are be unique. Thus, we must not
        // find dups.
        assert(!r);
    }


    // now search
    VECT_FOR_EACH(&v, p) {
        void *ret = 0;

        t0 = timenow();
        c0 = sys_cpu_timestamp();
        int r = ht_find(h, p->key, &ret);
        tf  += timenow() - t0;
        cyf += sys_cpu_timestamp() - c0;

        if (!r) {
            printf("key %#16.16llx not found!\n", p->key);
            ht_dump(h, "can't find key", dump);
            assert(r);
        }
        assert(ret == (void *)p->val);
    }

#define _d(x) ((double)(x))

    printf("Perf: %lld elems; %lld buckets, %lld filled (%5.2f %%)\n"
           "      %u max-bags/bucket, %u splits\n",
           h->nodes, h->n, h->fill, 100.0 * (_d(h->fill)/_d(h->n)),
           h->bagmax, h->splits);

    // now delete some of them.
    uint64_t ndel = 0;
    VECT_FOR_EACH(&v, p) {
        void *ret = 0;

        // delete odd valued keys
        if (p->key & 1) {
            ndel++;

            t0 = timenow();
            c0 = sys_cpu_timestamp();
            int r = ht_remove(h, p->key, &ret);

            td  += timenow() - t0;
            cyd += sys_cpu_timestamp() - c0;
            assert(r);
            assert(ret == (void *)p->val);
        }
    }

    // now search again.
    VECT_FOR_EACH(&v, p) {
        void *ret = 0;

        if (p->key & 1) {
            t0 = timenow();
            c0 = sys_cpu_timestamp();
            int r = ht_find(h, p->key, &ret);
            tx  += timenow() - t0;
            cyx += sys_cpu_timestamp() - c0;

            // we shouldn't find these keys
            assert(!r);
        } else {
            int r = ht_find(h, p->key, &ret);

            // even valued keys must be found
            assert(r);
            assert(ret == (void *)p->val);
        }
    }



    // timenow() returns time in nanoseconds
    // sys_cpu_timestamp() returns clock cycles (perf counter)


    // this captures speed of operation (N ns/op)
    double ispd = _d(ti) / _d(VECT_LEN(&v)),
           fspd = _d(tf) / _d(VECT_LEN(&v)),
           dspd = _d(td) / _d(ndel),
           xspd = _d(tx) / _d(ndel);


    // this calculates cycle count per op
    double clki = _d(cyi) / _d(VECT_LEN(&v)),
           clkf = _d(cyf) / _d(VECT_LEN(&v)),
           clkd = _d(cyd) / _d(ndel),
           clkx = _d(cyx) / _d(ndel);


    printf("probe:   %5.2f ns/insert %5.2f clks/insert\n"
           "find:    %5.2f ns/find %5.2f clks/find\n"
           "find-x:  %5.2f ns/find-non-exist %5.2f clks/find-non-exist\n"
           "del:     %5.2f ns/del %5.2f clks/del\n",
            ispd, clki,
            fspd, clkf,
            xspd, clkx,
            dspd, clkd);


    // cleanup
    ht_fini(h);
    VECT_FINI(&v);
}


int
main()
{
    basic_tests();
    rand_tests();
}