/*
 * Test harness for IP addr parsing.
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "error.h"

#include "utils/parse-ip.h"

// Return true if we can convert the address; false otherwise.
static int
std_ip4(const char* s, uint32_t *p_ret)
{
    struct in_addr a;
    int r =  inet_pton(AF_INET, s, &a);

    *p_ret = a.s_addr;
    return r;
}


struct iptest {
    const char* s;
    int       ret;
    uint32_t  exp_a, exp_m;
};
typedef struct iptest iptest;

const iptest T[] = {
      { "1.1.1.1",    1, 0x01010101, 0xffffffff }
    , { "0.0.0.0/32", 1, 0x00000000, 0xffffffff }
    , { "0.0.0.0",    1, 0x00000000, 0xffffffff }
    , { "255.255.255.255", 1, 0xffffffff, 0xffffffff }
    , { "1.1.1.1/24", 1, 0x01010101, 0xffffff00 }
    , { "22.22.22.22/16", 1, 0x16161616, 0xffff0000 }
    , { "3.3.3.3/8",  1, 0x03030303, 0xff000000 }
    , { "1.1.1.1/255.255.255.0", 1, 0x01010101, 0xffffff00 }


    // Negative cases
    , { "1.1.1.1/249",    0, 0x01010101, 0 }
    , { "1.1", 0, 0, 0 }
    , { "1.1.2.33.4", 0, 0, 0 }
    , { "1.1.1.1/249.22", 0, 0x01010101, 0 }
    , { "1.1.1.1/355.255.255.0",    0, 0x01010101, 0 }
    , { "1.1000.1.1/255.255.255.0", 0, 0x01010101, 0 }

    // Keep in the end
    , { 0, 0, 0, 0}
};

struct masktest {
    uint32_t  mask;
    uint32_t  exp;
    int ret;
};
typedef struct masktest masktest;

const masktest Tm[] = {
      { 0xffffffff, 32, 1}
    , { 0, 0,  1}
    , { 0xffffff00, 24, 1}
    , { 0xffff0000, 16, 1}
    , { 0xfffff800, 21, 1}

    // negative tests
    , { 0xffff8011, 0, 0}
    , { 0, 0, -1 }
};


static void
iptests()
{
    const iptest* t = T;
    uint32_t a, m;
    uint32_t x;

    for (; t->s; ++t) {
        a = m = 0;
        int r = parse_ipv4_and_mask(&a, &m, t->s);
        if (r != t->ret)   error(0, 0, "Retval fail <%s>; exp %d saw %d", t->s, t->ret, r);
        if (r) {
            if (a != t->exp_a) error(0, 0, "Addr fail   <%s>; exp %u saw %u", t->s, t->exp_a, a);
            if (m != t->exp_m) error(0, 0, "Mask fail   <%s>; exp %u saw %u", t->s, t->exp_m, m);
        }

        if (!strchr(t->s, '/')) {
            r = std_ip4(t->s, &x);
            if (r != t->ret) error(0, 0, "pton ret fail <%s>", t->s);
            if (r) {
                if (a != x)        error(0, 0, "addr fail <%s>; exp %u saw %u", t->s, a, x);
                if (x != t->exp_a) error(0, 0, "addr fail <%s>; exp %u saw %u", t->s, t->exp_a, x);
            }
        }
    }
}


static void
masktests()
{
    const masktest * t = Tm;

    for (; t->ret != -1; t++) {
        uint32_t p = 0;
        int r = mask_to_cidr4(t->mask, &p);

        if (r != t->ret)    error(0, 0, "retval fail <%x>; exp %d, saw %d", t->mask, t->ret, r);
        if (r) {
            if (p != t->exp) error(0, 0, "mask fail <%x>; exp %d, saw %d", t->mask, t->exp, p);
        }
    }
}


int
main()
{
    iptests();
    masktests();
    return 0;
}
