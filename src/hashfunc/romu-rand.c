/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * src/romu-rand.c - Implementation of romu pseudo random generators
 *
 * Copyright (c) 2020 Sudhi Herle <sw at herle.net>
 *
 * Licensing Terms: GPLv2 
 *
 * If you need a commercial license for this work, please contact
 * the author.
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 */

#include "utils/utils.h"
#include "utils/romu-rand.h"
#include "utils/splitmix.h"


void
romu_quad_init(romu *r, uint64_t a, uint64_t b)
{
    if (!a) a = makeseed();
    if (!b) b = makeseed();

    r->w = a;
    r->x = b;
    r->y = ~a;
    r->z = ~b;
}


void
romu_trio_init(romu *r, uint64_t a, uint64_t b)
{
    if (!a) a = makeseed();
    if (!b) b = makeseed();

    r->x = a;
    r->y = b;
    r->z = ~a;
    r->w = 0;
}


void
romu_duo_init(romu *r, uint64_t a)
{
    if (!a) a = makeseed();

    r->x = a;
    r->y = ~a;
    r->w = 0;
    r->z = 0;
}


uint64_t
romu_quad(romu *r)
{
    uint64_t w = r->w,
             x = r->x,
             y = r->y,
             z = r->z;

    r->w = 15241094284759029579u * z;
    r->x = z + rotl(w, 52);
    r->y = y - x;
    r->z = rotl(y+w, 19);
    return x;
}


uint64_t
romu_trio(romu *r)
{
    uint64_t x = r->x,
             y = r->y,
             z = r->z;

    r->x = 15241094284759029579u * z;
    r->y = rotl(y - x, 12);
    r->z = rotl(z - y, 44);
    return x;
}


uint64_t
romu_duo(romu *r)
{
    uint64_t x = r->x;

    r->x = 15241094284759029579u * r->y;
    r->y = rotl(r->y, 36) + rotl(r->y, 15) - x;
    return x;
}

/* EOF */
