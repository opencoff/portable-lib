/*
 * Simple test harness for the progress bar util
 */
#include <time.h>
#include "utils/progbar.h"

#define _S      1
#define _US     1000
#define _MS     1000000
#define _NS     1000000000


int
main()
{
    progress p;
    uint64_t tot = 80 * 1048576;


    struct timespec ts = {
        .tv_sec = 0,
        .tv_nsec = 100 * _MS,
    };

    uint64_t n = 0;
    uint64_t inc = 350 * 1024;

    progressbar_init(&p, 2, tot, P_HUMAN);
    while (n < tot) {
        progressbar_update(&p, inc);
        n += inc;
        nanosleep(&ts, 0);
    }

    progressbar_finish(&p, 0, 1);
}
