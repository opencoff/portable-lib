/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * utils.h - simple C utilities
 *
 * Copyright (c) 2005 Sudhi Herle <sw at herle.net>
 *
 * Licensing Terms: GPLv2 
 *
 * If you need a commercial license for this work, please contact
 * the author.
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 *
 * Creation date: Sat Jan 21 18:09:15 2006
 */

#ifndef __UTILS_H_1137888555_79680__
#define __UTILS_H_1137888555_79680__ 1


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h> /* ptrdiff_t */
#include <time.h>
#include <sys/time.h>


#if defined(_WIN32) || defined(WIN32)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Return basename of a path. The return value will be pointer to
 * the basename component of 'p'. i.e., no new string will be
 * allocated.
 *
 * @param p - path whose basename will be returned. 
 */

extern const char* basename(const char* p);

/**
 * Return dirname of a path. This function will _modify_ the
 * original string and return a pointer to the dirname.
 *
 * @param p - path whose dirname will be returned.
 */
extern char* dirname(char* p);

extern int strcasecmp(const char* src, const char* dest);

#ifdef __cplusplus
}
#endif

#else

#include <strings.h>    /* strcasecmp */
#include <libgen.h>     /* basename, dirname */

#endif /* WIN32 */


    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Handy macro to allocate an instance of a particular type */
#define NEW(ty_)            (ty_ *)malloc(sizeof(ty_))

/* Macro to allocate an array of 'ty_' type elements. */
#define NEWA(ty_,n)         (ty_ *)malloc(sizeof (ty_) * (n))

/* Macro to allocate a zero initialized item of type 'ty_' */
#define NEWZ(ty_)           (ty_ *)calloc(1, sizeof (ty_))

/* Macro to allocate a zero initialized array of type 'ty_' items */
#define NEWZA(ty_,n)        (ty_ *)calloc((n), sizeof (ty_))

/* Macro to 'realloc' an array 'ptr' of items of type 'ty_' */
#define RENEWA(ty_,ptr,n)   (ty_ *)realloc(ptr, (n) * sizeof(ty_))

/* Crazy macro to pretend we use an arg */
#define USEARG(x)          (void)(x)

/* Corresponding macro to "free" an item obtained via NEWx macros.
 * */
#define DEL(ptr)             do { \
                                if (ptr) free(ptr); \
                                ptr = 0; \
                            } while (0)

/* Return the size of a initialized array of any type */
#define ARRAY_SIZE(a)       (sizeof(a) / sizeof(a[0]))



/* Functional style casts */
#define pUCHAR(x)    ((unsigned char*)(x))
#define pUINT(x)     ((unsigned int*)(x))
#define pULONG(x)    ((unsigned long*)(x))
#define pCHAR(x)     ((char*)(x))
#define pINT(x)      ((int*)(x))
#define pLONG(x)     ((long*)(x))

#define _ULLONG(x)   ((unsigned long long)(x))
#define _ULONG(x)    ((unsigned long)(x))
#define _UINT(x)     ((unsigned int)(x))
#define _PTRDIFF(x)  ((ptrdiff_t)(x))

#define pU8(x)       ((uint8_t *)(x))
#define pU16(x)      ((uint16_t *)(x))
#define pU32(x)      ((uint32_t *)(x))
#define pU64(x)      ((uint64_t *)(x))

#define pVU8(x)      ((volatile uint8_t *)(x))
#define pVU16(x)     ((volatile uint16_t *)(x))
#define pVU32(x)     ((volatile uint32_t *)(x))
#define pVU64(x)     ((volatile uint64_t *)(x))

#define _U8(x)       ((uint8_t)(x))
#define _U16(x)      ((uint16_t)(x))
#define _U32(x)      ((uint32_t)(x))
#define _U64(x)      ((uint64_t)(x))

#define _VU8(x)      ((volatile uint8_t)(x))
#define _VU16(x)     ((volatile uint16_t)(x))
#define _VU32(x)     ((volatile uint32_t)(x))
#define _VU64(x)     ((volatile uint64_t)(x))



/* Align a quantity 'v' to 'n' byte boundary and return as type 'T". */
#define _ALIGN_UP(v, n)        (typeof(v))({ \
                                  uint64_t x_ = _U64(v); \
                                  uint64_t z_ = _U64(n) - 1; \
                                  (x_ + z_) & ~z_;})
                            

#define _ALIGN_DOWN(v, n)    (typeof(v))({ \
                                uint64_t x_ = _U64(v); \
                                uint64_t z_ = _U64(n) - 1; \
                                x_ & ~z_;})

#define _IS_ALIGNED(v, n)    ({ \
                                uint64_t x_  = _U64(v); \
                                uint64_t z_ = _U64(n) - 1; \
                                x_ == (x_ & ~z_);})
                            
#define _IS_POW2(n)          ({ \
                                uint64_t x_ = _U64(n); \
                                x_ == (x_ & ~(x_-1));})


/**
 * dump 'buflen' bytes of buffer in 'inbuf' as a series of hex bytes.
 * For each 16 bytes, call 'output' with the supplied cookie as the
 * first argument.
 *
 * @param inbuf     The input buffer to dump
 * @param buflen    Number of bytes to dump
 * @param output    Output function pointer to output hex bytes
 * @param cookie    Opaque cookie to be passed to output function
 * @param msg       Message to be prefixed to the dump
 * @param offsetonly Flag - if set will only print the offsets and
 *                   not actual pointer addresses.
 */
extern void hex_dump_bytes(const void* inbuf, int buflen,
                void (*output)(void*, const char* line, int nbytes),
                void* cookie, const char* msg, int offsetonly);


/**
 * Dump 'inlen' bytes from 'inbuf' into 'buf' which is 'bufsize'
 * bytes big.
 *
 * @param inbuf     The input buffer to dump
 * @param inlen     Number of bytes to dump
 * @param buf       Output buffer to hold the hex dump
 * @param bufsize   Size of output buffer
 * @param msg       Message to be prefixed to the dump
 * @param offsetonly Flag - if set will only print the offsets and
 *                   not actual pointer addresses.
 */
extern void hex_dump_buf(char *buf, int bufsize, const void* inbuf, int inlen, const char* msg, int offsetonly);


/**
 * dump 'buflen' bytes of buffer in 'inbuf' as a series of ascii hex
 * bytes to file 'fp'.
 *
 * @param offsetonly Flag - if set will only print the offsets and
 *                   not actual pointer addresses.
 */
extern void hex_dump_bytes_file(const void* inbuf, int buflen, FILE* fp, const char* msg, int offsetonly);




/**
 * Trim a string by removing all leading and trailing white-spaces.
 */
extern char* strtrim(char* s);




/**
 * Split a string delimited by "tokens" into its constituent parts.
 * Returns a newly allocated array of strings - each of which point
 * into the original string. It is the caller's responsibility to
 * free this new array.
 *
 * NB: 'str' - the input string needs to be WRITABLE. The delimiters
 *     are replaced by '\0'. i.e., the array of strings point to
 *     places inside 'str' where the substrings begin.
 */
extern char** strsplit(int *strv_size, char* str, const char* tok, int sqz_consec);


/**
 * Split a string delimited by "tokens" into its constituent parts.
 * Fills the array 'strv' with pointers into the input string 'str'.
 * Returns:
 *
 *      < 0: if the array size was insufficient to hold all the
 *           constituent parts
 *      >=0: Number of split tokens in the original string; this
 *           number is necessarily <= n.
 *
 * NB: 'str' - the input string needs to be WRITABLE. The delimiters
 *     are replaced by '\0'.
 */
extern int  strsplit_quick(char** strv, int n, char* str, const char* tok, int sqz_consec);


/**
 * Split a Comma separated string, handle quoted strings and
 * escaped quotes.
 *
 * The separator can be any of characters in 'sep'. A nil ptr
 * indicates the _default_ of ','.  If 'sep' contains more than
 * one character, then any of them can be a valid separator.
 * e.g., ",;" indicates that ',' or ';' can be a field separator.
 *
 * Fills the array 'strv' with pointers into the input string 'str'.
 * Returns:
 *
 *      < 0: -ENOSPC if array is too small to hold the delimited parts
 *      >=0: Number of split tokens in the original string; this
 *           number is necessarily <= n.
 *
 * NB: 'str' - the input string needs to be WRITABLE. The delimiters
 *     are replaced by '\0'.
 */
extern int  strsplit_csv(char** strv, int n, char* str, const char *sep);


/**
 * Split a string into tokens just as shell would; handle quoted
 * words as a single word. Uses whitespace (' ', '\t') as delimiter
 * tokens. Delimited tokens are stuffed in 'args'.
 *
 * Returns:
 *
 *    >= 0  number of parsed tokens
 *    < 0   on error:
 *          -ENOSPC if array is too small
 *          -EINVAL if ending quote is missing
 */
extern int strsplitargs(char **args, size_t n, char *s);


/**
 * Convert a string 's' to a uint64_t and detect overflows.
 * Set '*endptr' to the last unconverted character. The converted
 * uint64_t is in 'p_ret'. If 'base' is 0, then auto-detect the
 * conversion base; else use what is provided.
 *
 * Returns:
 *      0   No error
 *    < 0   errno
 *          -ERANGE if string is too long and overflows uint64_t
 *          -EINVAL if string has invalid characters for the given base 
 *                  or if the base is invalid (> 36).
 */
extern int strtou64(const char *s, const char **endptr, int base, uint64_t *p_ret);


/**
 * Parse a string 'str' containing a size suffix. The suffix has the
 * following meaning:
 *      B       bytes
 *      k, K    Kilobyte (1024)
 *      M       Megabyte (1024 * K)
 *      G       Gigabyte (1024 * M)
 *      T       Terabyte (1024 * G)
 *      P       Petabyte (1024 * T)
 *      E       Exabyte  (1024 * E)
 *
 * If the size suffix is followed by a lower case 'b', then it is
 * the suffix multiplier is treated as "bits". e.g., "Kb" is parsed
 * as "Kilo bit". Similarly, an upper case 'B' is treated as "byte".
 * This is the default interepretation. e.g., "1000M" is interpreted as
 * "1000 Mega bytes".
 *
 * Returns the parsed string in 'p_val'
 *
 * Returns:
 *      == 0: If parsing is successful
 *      <  0: -Errno on error
 */
extern int strtosize(const char* str, int base, uint64_t* val);



/**
 * Remove leading & trailing quote chars from the string.
 * If 'p_ret' is non-Null, return the resulting substring in
 * '*p_ret'.
 *
 * Return:
 *    0     No quote found
 *    > 0   Quote char that was removed (either '"' or '\'')
 *    < 0   No ending quote found
 *
 * NB: This function expects the caller to have already trimmed
 *     leading/trailing white spaces.
 */
extern int strunquote(char* s, char **p_ret);


/**
 * Robust readline() that handles CR, LF, CRLF line break
 * characters.
 * Returns:
 *    > 0: Length of the string
 *    0:   EOF
 *    -ENOSPC: Input buffer too small
 *    < 0: -errno for all other errors
 */
extern int freadline(FILE* fp, unsigned char * str, int n);



/**
 * Print 'n' in human readable units (understands upto EB).
 * Returns:
 *    original output buffer 'buf'
 */
extern char* humanize_size(char * buf, size_t bufsize, uint64_t n);



/**
 * Safe strcopy():
 *   - doesn't write more than 'sz' bytes into 'dest'
 *   - null terminates at all times
 *
 * Returns:
 *   >= 0  number of bytes copied
 *   < 0   truncated
 */
extern ssize_t strcopy(char* dest, size_t sz, const char* src);


/**
 * Decode hex chars in string 'str' and put them into 'out'.
 *
 * Return:
 *    < 0   -EINVAL Invalid hex chars in string
 *          -ENOMEM If out is too small to hold decoded string
 *    > 0   Number of decoded bytes.
 */
extern ssize_t str2hex(uint8_t* out, size_t outlen, const char* str);



/*
 * Other utility functions
 */


#ifdef __GNUC__
#define likely(x)     __builtin_expect(!!(x),1)
#define unlikely(x)   __builtin_expect(!!(x),0)
#else
#define likely(x)      (x)
#define unlikely(x)    (x)
#endif /* __GNUC__ */



static inline uint64_t
timenow()
{
    struct timeval tv;
    gettimeofday(&tv, 0);

    return tv.tv_usec + (1000000 * tv.tv_sec);
}


/*
 * Round 'v' to the next closest power of 2.
 */
#define NEXTPOW2(v)     ({\
                            uint64_t n_ = _U64(v)-1;\
                            n_ |= n_ >> 1;      \
                            n_ |= n_ >> 2;      \
                            n_ |= n_ >> 4;      \
                            n_ |= n_ >> 8;      \
                            n_ |= n_ >> 16;     \
                            n_ |= n_ >> 32;     \
                            (typeof(v))(n_+1);  \
                      })



/*
 * Performance counter access.
 *
 * NB: Relative cycle counts and difference between two
 *     cpu-timestamps are meaningful ONLY when run on the _same_ CPU.
 */
#if defined(__i386__)

static inline uint64_t sys_cpu_timestamp(void)
{
    uint64_t x;
    __asm__ volatile ("rdtsc" : "=A" (x));
    return x;
}

#elif defined(__x86_64__)


static inline uint64_t sys_cpu_timestamp(void)
{
    uint64_t res;
    uint32_t hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));

    res = hi;
    return (res << 32) | lo;
}

#else

#error "I don't know how to get CPU cycle counter for this machine!"

#endif /* x86, x86_64 */



#ifdef __cplusplus
}

#include <cassert>

namespace putils {


/* 
 * Some templates to make life easier
 */

// Simple template to align a qty to a desired boundary

// align 'p' to next upward boundary of 'n'
// i.e., the return value may be larger than 'p' but never less than
// 'p'
template <typename T> inline T
align_up(T p, unsigned long n)
{
    // n must be power of two
    assert(n == (n & ~(n-1)));
    ptrdiff_t v = (ptrdiff_t(p) + (n-1)) & ~(n-1);

    return T(v);
}


// align 'p' to nearest downward boundary of 'n'
// i.e., the return value will not be larger than 'p'
template <typename T> inline T
align_down(T p, unsigned long n)
{
    // n must be power of two
    assert(n == (n & ~(n-1)));
    ptrdiff_t v = ptrdiff_t(p) & ~(n-1);

    return T(v);
}


// Return true if 'p' is aligned at the 'n' byte boundary and false
// otherwise
template <typename T> inline bool
is_aligned(T p, unsigned long n)
{
    // n must be power of two
    assert(n == (n & ~(n-1)));
    return uint64_t(p) == (uint64_t(p) & ~uint64_t(n-1));
}

template <typename T> inline T
round_up_pow2(T n)
{
    n--;
    switch (sizeof(T))
    {
        case 1:
            n |= n >> 1;
            n |= n >> 2;
            n |= n >> 4;
            break;

        case 2:
            n |= n >> 1;
            n |= n >> 2;
            n |= n >> 4;
            n |= n >> 8;
            break;

        default:
        case 4:
            n |= n >> 1;
            n |= n >> 2;
            n |= n >> 4;
            n |= n >> 8;
            n |= n >> 16;
            break;

        case 8:
            n |= n >> 1;
            n |= n >> 2;
            n |= n >> 4;
            n |= n >> 8;
            n |= n >> 16;
            n |= n >> 32;
            break;
    }
    return n+1;
}


} // namespace putils

#endif /* __cplusplus */

#endif /* ! __UTILS_H_1137888555_79680__ */

/* EOF */
