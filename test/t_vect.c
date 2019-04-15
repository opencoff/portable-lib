/*
 * Test harness for fast/vect.h
 */


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "fast/vect.h"

struct zz
{
    int v0;
    int v1;
};
typedef struct zz zz;

// array of structs
VECT_TYPEDEF(vect, zz);

extern uint32_t arc4random(void);

#define zzz(x)  ({zz _z = {.v0=x, .v1=x*10}; _z;})

static int
findz(vect *vz, int i)
{
    zz* z;

    VECT_FOR_EACH(vz, z) {
        if (z->v0 == i) return 1;
    }
    return 0;
}


int
main()
{
    size_t i;
    vect  vz;
    zz z;

    VECT_INIT(&vz, 4);

    VECT_PUSH_BACK(&vz, zzz(0));
    VECT_PUSH_BACK(&vz, zzz(1));
    VECT_PUSH_BACK(&vz, zzz(2));
    VECT_PUSH_BACK(&vz, zzz(3));
    VECT_PUSH_BACK(&vz, zzz(4));
    VECT_PUSH_BACK(&vz, zzz(5));
    VECT_PUSH_BACK(&vz, zzz(6));
    VECT_PUSH_BACK(&vz, zzz(7));
    VECT_PUSH_BACK(&vz, zzz(8));
    VECT_PUSH_BACK(&vz, zzz(9));
    VECT_PUSH_BACK(&vz, zzz(10));

    assert(VECT_LEN(&vz) == 11);
    for (i = 0; i < 10; i++) {
        assert(findz(&vz, i));
    }

    z = VECT_POP_BACK(&vz);
    assert(VECT_LEN(&vz) == 10);
    assert(z.v0 == 10 && z.v1 == 100);

    VECT_PUSH_FRONT(&vz, zzz(33));
    assert(VECT_LEN(&vz) == 11);
    z = VECT_FIRST_ELEM(&vz);
    assert(z.v0 == 33 && z.v1 == 330);

    z = VECT_POP_FRONT(&vz);
    assert(VECT_LEN(&vz) == 10);
    assert(z.v0 == 33 && z.v1 == 330);

    z = VECT_FIRST_ELEM(&vz);
    assert(z.v0 == 0 && z.v1 == 0);

    vect a;
    vect b;
    VECT_INIT(&a, 4);
    VECT_INIT(&b, 8);
    VECT_PUSH_BACK(&a, zzz(11));
    VECT_PUSH_BACK(&a, zzz(12));
    VECT_PUSH_BACK(&a, zzz(13));

    assert(VECT_LEN(&a) == 3);
    assert(VECT_LEN(&b) == 0);

    VECT_COPY(&b, &a);
    assert(VECT_LEN(&b) == 3);
    for (i = 0; i < VECT_LEN(&a); i++) {
        assert(findz(&b, VECT_ELEM(&a, i).v0));
    }
    VECT_FINI(&b);

    VECT_APPEND_VECT(&vz, &a);
    assert(VECT_LEN(&vz) == 13);

    VECT_SHUFFLE(&vz, arc4random);
    for (i = 0; i < VECT_LEN(&a); i++) {
        assert(findz(&vz, VECT_ELEM(&a, i).v0));
    }

    vect s;

    VECT_INIT(&s, 4);

    VECT_SAMPLE(&s, &vz, 3, arc4random);
    zz *x;
    VECT_FOR_EACHi(&s, i, x) {
        printf("Sample %2zd: %d.%d\n", i, x->v0, x->v1);
    }

    return 0;
}
