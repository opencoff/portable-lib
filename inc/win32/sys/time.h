/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * sys/time.h - Win32 compatibility header file for sys/time.h
 *
 * (c) 2005 Sudhi Herle <sw at herle.net>
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
 *
 * Creation date: Sun Sep 11 12:58:43 2005
 */
#ifndef __SYS_TIME_H__55631xyz__
#define __SYS_TIME_H__55631xyz__ 1

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifndef _TIMEVAL_DEFINED /* also in winsock[2].h */
#define _TIMEVAL_DEFINED
struct timeval {
  long tv_sec;
  long tv_usec;
};
#endif /* _TIMEVAL_DEFINED */


#ifndef timerisset
#define timerisset(tvp)	 ((tvp)->tv_sec || (tvp)->tv_usec)
#endif

#ifndef timercmp
#define timercmp(tvp, uvp, cmp) \
	(((tvp)->tv_sec != (uvp)->tv_sec) ? \
	((tvp)->tv_sec cmp (uvp)->tv_sec) : \
	((tvp)->tv_usec cmp (uvp)->tv_usec))
#endif

#ifndef timerclear
#define timerclear(tvp)	 (tvp)->tv_sec = (tvp)->tv_usec = 0
#endif

#ifndef timeradd
# define timeradd(a, b, result)                                               \
  do {                                                                        \
    (result)->tv_sec = (a)->tv_sec + (b)->tv_sec;                             \
    (result)->tv_usec = (a)->tv_usec + (b)->tv_usec;                          \
    if ((result)->tv_usec >= 1000000)                                         \
      {                                                                       \
        ++(result)->tv_sec;                                                   \
        (result)->tv_usec -= 1000000;                                         \
      }                                                                       \
  } while (0)
#endif

#ifndef timersub
# define timersub(a, b, result)                                               \
  do {                                                                        \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;                             \
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;                          \
    if ((result)->tv_usec < 0) {                                              \
      --(result)->tv_sec;                                                     \
      (result)->tv_usec += 1000000;                                           \
    }                                                                         \
  } while (0)
#endif


#ifndef timespeccmp
#define timespeccmp(tvp, uvp, cmp) \
	(((tvp)->tv_sec != (uvp)->tv_sec) ? \
	((tvp)->tv_sec cmp (uvp)->tv_sec) : \
	((tvp)->tv_nsec cmp (uvp)->tv_nsec))
#endif

#ifndef timespecclear
#define timespecclear(tvp)	 (tvp)->tv_sec = (tvp)->tv_nsec = 0
#endif

#ifndef timespecadd
# define timespecadd(a, b, result)                                             \
  do {                                                                        \
    (result)->tv_sec = (a)->tv_sec + (b)->tv_sec;                             \
    (result)->tv_nsec = (a)->tv_nsec + (b)->tv_nsec;                          \
    if ((result)->tv_nsec >= 1000000000)                                      \
      {                                                                       \
        ++(result)->tv_sec;                                                   \
        (result)->tv_nsec -= 1000000000;                                      \
      }                                                                       \
  } while (0)
#endif

#ifndef timespecsub
# define timespecsub(a, b, result)                                             \
  do {                                                                        \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;                             \
    (result)->tv_nsec = (a)->tv_nsec - (b)->tv_nsec;                          \
    if ((result)->tv_nsec < 0) {                                              \
      --(result)->tv_sec;                                                     \
      (result)->tv_nsec += 1000000000;                                        \
    }                                                                         \
  } while (0)
#endif

/* obsolete BSD Definition */
struct timezone
{
    int tz_minuteswest;         /* Minutes west of GMT.  */
    int tz_dsttime;             /* Nonzero if DST is ever in effect.  */
};


struct timespec
{
    unsigned long tv_sec;
    long tv_nsec;
};


/* Macros for converting between `struct timeval' and `struct timespec'.  */
# define TIMEVAL_TO_TIMESPEC(tv, ts) {                                   \
        (ts)->tv_sec = (tv)->tv_sec;                                    \
        (ts)->tv_nsec = (tv)->tv_usec * 1000;                           \
}
# define TIMESPEC_TO_TIMEVAL(tv, ts) {                                   \
        (tv)->tv_sec = (ts)->tv_sec;                                    \
        (tv)->tv_usec = (ts)->tv_nsec / 1000;                           \
}


extern int gettimeofday(struct timeval * tv, struct timezone * __unused);
extern int gettimeofday_ns(struct timespec * tv, struct timezone * __unused);

#ifdef __cplusplus
}
#endif

#endif  /* __SYS_TIME_H__55631xyz__ */
