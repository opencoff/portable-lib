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
VECT_TYPEDEF(zz_vect, zz);

extern uint32_t arc4random(void);

#define zzz(x)  ({zz _z = {.v0=x, .v1=x*10}; _z;})

static int
findz(zz_vect *vz, int i)
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
    zz_vect  vz;

    VECT_INIT(&vz, 4);

    VECT_APPEND(&vz, zzz(0));
    VECT_APPEND(&vz, zzz(1));
    VECT_APPEND(&vz, zzz(2));
    VECT_APPEND(&vz, zzz(3));
    VECT_APPEND(&vz, zzz(4));
    VECT_APPEND(&vz, zzz(5));
    VECT_APPEND(&vz, zzz(6));
    VECT_APPEND(&vz, zzz(7));
    VECT_APPEND(&vz, zzz(8));
    VECT_APPEND(&vz, zzz(9));
    VECT_APPEND(&vz, zzz(10));

    VECT_SHUFFLE(&vz, arc4random);

    // Now how the heck do we verify this?!
    assert(findz(&vz, 0));
    assert(findz(&vz, 1));
    assert(findz(&vz, 2));
    assert(findz(&vz, 3));

    assert(VECT_SIZE(&vz) == 11);

    VECT_SHUFFLE(&vz, arc4random);

    zz_vect s;

    VECT_INIT(&s, 4);

    VECT_SAMPLE(&s, &vz, 3, arc4random);
    zz *x;
    size_t i;
    VECT_FOR_EACHi(&s, i, x) {
        printf("Sample %2zd: %d.%d\n", i, x->v0, x->v1);
    }

    return 0;
}
