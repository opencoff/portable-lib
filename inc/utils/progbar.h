/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * utils/progbar.h - Simple progress bar
 *
 * Copyright (c) 2019 Sudhi Herle <sw at herle.net>
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

#ifndef ___INC_UTILS_PROGBAR_H__ZE4uqtyjdQsGn5Jg___
#define ___INC_UTILS_PROGBAR_H__ZE4uqtyjdQsGn5Jg___ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <inttypes.h>

struct progress {
    int fd;
    uint64_t cur;
    uint64_t total;
    uint32_t lines;

    uint32_t step;
    uint32_t width;

    uint32_t flags;

    char buf[512];
};
typedef struct progress progress;

// Print current & total in human units (kB/MB/GB etc.)
#define P_HUMAN     (1 << 0)


/*
 * Initialize a progress bar instance to write to 'fd'.
 * If 'total' is non zero, then an actuall progress bar with
 * completion percent is drawn. 
 * If 'total' is zero, then a simple progress bar to count the
 * current value is shown.
 *
 * If 'fd' is NOT a tty, then no progress is shown.
 */
int progressbar_init(progress*, int fd, uint64_t total, uint32_t flags);

/*
 * Update the progress bar with an incremental value of 'incr'
 */
void progressbar_update(progress*, uint64_t incr);

/*
 * Finish/complete the progress bar by writing a '\n' to the output.
 * If 'clr' is true, then erase the current line. Else, write '\n'.
 * if 'newln' is true, write a '\n'.
 */
void progressbar_finish(progress*, int clr, int newln);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___INC_UTILS_PROGBAR_H__ZE4uqtyjdQsGn5Jg___ */

/* EOF */
