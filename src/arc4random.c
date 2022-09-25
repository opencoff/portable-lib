/*
 * Copyright (c) 1996, David Mazieres <dm@uun.org>
 * Copyright (c) 2008, Damien Miller <djm@openbsd.org>
 * Copyright (c) 2013, Markus Friedl <markus@openbsd.org>
 * Copyright (c) 2014, Theo de Raadt <deraadt@openbsd.org>
 * Copyright (c) 2015, Sudhi Herle   <sudhi@herle.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */


/*
 * ChaCha based random number generator from OpenBSD.
 *
 * Made fully portable and thread-safe by Sudhi Herle.
 */

#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <assert.h>
#include <pthread.h>


#define ARC4R_KEYSZ     32
#define ARC4R_IVSZ      8
#define ARC4R_BLOCKSZ   64
#define ARC4R_RSBUFSZ   (16*ARC4R_BLOCKSZ)

// must be a power of 2
#define REKEY_BASE      (1 << 10)

typedef struct
{
    uint32_t input[16]; /* could be compressed */
} chacha_ctx;

struct rand_state
{
    size_t          rs_have;    /* valid bytes at end of rs_buf */
    size_t          rs_count;   /* bytes till reseed */
    pid_t           rs_pid;     /* My PID */
    chacha_ctx      rs_chacha;  /* chacha context for random keystream */
    uint8_t         rs_buf[ARC4R_RSBUFSZ];  /* keystream blocks */
};
typedef struct rand_state rand_state;


/* kernel entropy */
extern int getentropy(void* buf, size_t n);


#define KEYSTREAM_ONLY
#include "chacha_private.h"

#define minimum(a, b) ((a) < (b) ? (a) : (b))

#include "utils/arc4random.h"




static inline void
_rs_init(rand_state* st, uint8_t *buf, size_t n)
{
    assert(n >= (ARC4R_KEYSZ + ARC4R_IVSZ));

    chacha_keysetup(&st->rs_chacha, buf, ARC4R_KEYSZ * 8, 0);
    chacha_ivsetup(&st->rs_chacha,  buf + ARC4R_KEYSZ);
}



static inline void
_rs_rekey(rand_state* st, uint8_t *dat, size_t datlen)
{
    /* fill rs_buf with the keystream */
    chacha_encrypt_bytes(&st->rs_chacha, st->rs_buf, st->rs_buf, sizeof st->rs_buf);

    /* mix in optional user provided data */
    if (dat) {
        size_t m = minimum(datlen, ARC4R_KEYSZ + ARC4R_IVSZ);
        for (size_t i = 0; i < m; i++)
            st->rs_buf[i] ^= dat[i];

        memset(dat, 0, datlen);
    }

    assert(sizeof(st->rs_buf) > (ARC4R_KEYSZ+ARC4R_IVSZ));

    /*
     * immediately reinit for backtracking resistance:
     * We're consuming enough bytes to make a new chacha context (key+iv).
     * Thus, we have those many fewer bytes in the entropy buf
     */
    _rs_init(st, st->rs_buf, ARC4R_KEYSZ + ARC4R_IVSZ);
    memset(st->rs_buf, 0, ARC4R_KEYSZ + ARC4R_IVSZ);
    st->rs_have = (sizeof st->rs_buf) - ARC4R_KEYSZ - ARC4R_IVSZ;
}


static void
_rs_stir(rand_state* st)
{
    uint8_t rnd[ARC4R_KEYSZ + ARC4R_IVSZ];
    size_t rekey_fuzz = 0;
    uint8_t *fuzzp    = (uint8_t*)&rekey_fuzz;


    int r = getentropy(rnd, sizeof rnd);
    assert(r == 0);

    _rs_rekey(st, rnd, sizeof(rnd));

    /* invalidate rs_buf */
    st->rs_have = 0;
    memset(st->rs_buf, 0, sizeof st->rs_buf);

    chacha_encrypt_bytes(&st->rs_chacha, fuzzp, fuzzp, sizeof rekey_fuzz);
    st->rs_count = REKEY_BASE + (rekey_fuzz % REKEY_BASE);
}


static inline void
_rs_stir_if_needed(rand_state* st, size_t len)
{
    if (st->rs_count <= len)
        _rs_stir(st);

    st->rs_count -= len;
}

// consume 'n' random btyes from the entropy buffer. Return total
// actually consumed.
static inline size_t
_rs_consume(rand_state *rs, uint8_t *buf, size_t n)
{
    size_t m    = minimum(n, rs->rs_have);
    uint8_t *ks = rs->rs_buf + sizeof(rs->rs_buf) - rs->rs_have;
    memcpy(buf, ks, m);
    memset(ks,  0,  m);
    rs->rs_have -= m;
    return m;
}

static inline void
_rs_random_buf(rand_state* rs, void *_buf, size_t n)
{
    uint8_t *buf = (uint8_t *)_buf;

    _rs_stir_if_needed(rs, n);
    while (n > 0) {
        if (rs->rs_have > 0) {
            size_t m = _rs_consume(rs, buf, n);
            buf += m;
            n   -= m;
        } else 
            _rs_rekey(rs, NULL, 0);
    }
}


static inline void
_rs_setup(rand_state *rs)
{
    uint8_t rnd[ARC4R_KEYSZ + ARC4R_IVSZ];
    int r = getentropy(rnd, sizeof rnd);
    assert(r == 0);

    _rs_init(rs, rnd, sizeof rnd);
    _rs_rekey(rs, 0, 0);
}


/* allocate a new rand_state and initialize it. */
static rand_state*
_rs_new(void)
{
    rand_state *rs = (rand_state*)calloc(sizeof *rs, 1);
    assert(rs);

    _rs_setup(rs);
    return rs;
}

#if defined(__Darwin__) || defined(__APPLE__)

/*
 * Multi-threaded support using pthread API. Needed for OS X:
 *
 *   https://www.reddit.com/r/cpp/comments/3bg8jc/anyone_know_if_and_when_applexcode_will_support/
 */
static pthread_key_t     Rkey;
static pthread_once_t    Ronce   = PTHREAD_ONCE_INIT;
static volatile uint32_t Rforked = 0;

/*
 * Fork handler to reset my context
 */
static void
atfork()
{
    // the pthread_atfork() callbacks called once per process.
    // We set it to be called by the child process.
    Rforked++;
}

/*
 * Run once and only once by pthread lib. We use the opportunity to
 * create the thread-specific key.
 */
static void
screate()
{
    pthread_key_create(&Rkey, 0);
    pthread_atfork(0, 0, atfork);
}


/*
 * Get the per-thread rand state. Initialize if needed.
 */
static rand_state*
sget()
{
    pthread_once(&Ronce, screate);

    volatile pthread_key_t* k = &Rkey;
    rand_state * z = (rand_state *)pthread_getspecific(*k);
    if (!z) {
        z = _rs_new();
        z->rs_pid = getpid();

        pthread_setspecific(*k, z);
    }

    /* Detect if a fork has happened */
    if (Rforked > 0 || getpid() != z->rs_pid) {
        Rforked   = 0;
        z->rs_pid = getpid();
        _rs_setup(z);
    }

    return z;
}

#else

/*
 * Use gcc extension to declare a thread-local variable.
 *
 * On most systems (including x86_64), thread-local access is
 * essentially free for non .so use cases.
 *
 */
static __thread rand_state st = { .rs_count = 0, .rs_pid = -1, .rs_have = 0 };
static inline rand_state*
sget()
{
    rand_state* s = &st;

    if (s->rs_count == 0 || getpid() != s->rs_pid) {
        _rs_setup(s);
        s->rs_pid = getpid();
    }
    return s;
}

#endif /* __Darwin__ */


/*
 * Public API.
 */


static inline uint32_t
__rand32(rand_state *z)
{
    uint32_t v;
    _rs_random_buf(z, &v, sizeof v);
    return v;
}


uint32_t
arc4random()
{
    rand_state* z = sget();
    return __rand32(z);
}


void
arc4random_buf(void* b, size_t n)
{
    rand_state* z = sget();

    _rs_random_buf(z, b, n);
}




/*
 * Calculate a uniformly distributed random number less than upper_bound
 * avoiding "modulo bias".
 *
 * Uniformity is achieved by generating new random numbers until the one
 * returned is outside the range [0, 2**32 % upper_bound).  This
 * guarantees the selected random number will be inside
 * [2**32 % upper_bound, 2**32) which maps back to [0, upper_bound)
 * after reduction modulo upper_bound.
 */
uint32_t
arc4random_uniform(uint32_t upper_bound)
{
    rand_state* z = sget();
    uint32_t r, min;

    if (upper_bound < 2)
        return 0;

    /* 2**32 % x == (2**32 - x) % x */
    min = -upper_bound % upper_bound;

    /*
     * This could theoretically loop forever but each retry has
     * p > 0.5 (worst case, usually far better) of selecting a
     * number inside the range we need, so it should rarely need
     * to re-roll.
     */
    for (;;) {
        r = __rand32(z);
        if (r >= min)
            break;
    }

    return r % upper_bound;
}

/* EOF */
