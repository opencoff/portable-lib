/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * src/progbar.c - Simple progress bar.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <assert.h>

#include "utils/strutils.h"
#include "utils/progbar.h"


// ANSI/VT terminal sequence to clear the line and position cursor
// at start of line.
// 0x1B[2k => ESC[2K
#define CLR     "\x1B[2K\r"

static void update_total_progress(char *buf, size_t bsize, progress *p);
static void update_incr_progress(char *buf, size_t bsize, progress *p);

int
progressbar_init(progress *p, int fd, uint64_t total, uint32_t flags)
{
    memset(p, 0, sizeof *p);

    if (fd < 0 || !isatty(fd)) {
        p->notty = 1;
        return 0;
    }

    p->total = total;
    p->fd = fd;
    p->cur = 0;
    p->flags = flags;

    // XXX Read tty width and choose
    p->width = 50;
    p->step  = 100 / p->width;

    return 0;
}


void
progressbar_update(progress *p, uint64_t incr)
{
    char buf[512];

    if (p->notty) return;

    p->cur += incr;

    if (p->total > 0) {
        update_total_progress(buf, sizeof buf, p);
    } else {
        update_incr_progress(buf, sizeof buf, p);
    }

    if (0 != strcmp(p->buf, buf)) {
        size_t n = strcopy(p->buf, sizeof p->buf, buf);

        if (p->lines++ > 0) {
            write(p->fd, CLR, strlen(CLR));
        }
        write(p->fd, p->buf, n);
    }

}

void
progressbar_finish(progress *p, int clr)
{
    if (!p->notty) {
        // do stuff ..
    }

    if (clr > 0) write(p->fd, CLR, strlen(CLR));

    write(p->fd, "\n", 1);
    p->cur = 0;
}


// Update 'buf' with a total progress bar with completion percent
// etc.
static void
update_total_progress(char *buf, size_t bsize, progress *p)
{
    char fill[p->width*2];
    char blank[p->width*2];
    char cur[32];
    char tot[32];
    char pref[64];

    uint64_t pct = (100 * p->cur) / p->total;

    if (pct > 100) pct = 100;

    uint64_t done = pct / p->step;
    uint64_t rem  = p->width - done;

    assert(done < (sizeof fill)-1);
    assert(rem  < (sizeof blank)-1);

    memset(fill, 'o', done); fill[done] = 0;
    memset(blank, '.', rem); blank[rem] = 0;

    if (p->flags & P_HUMAN) {
        humanize_size(cur, sizeof cur, p->cur);
        humanize_size(tot, sizeof tot, p->total);
    } else {
        snprintf(cur, sizeof cur, "%5" PRIu64 "", p->cur);
        snprintf(tot, sizeof tot, "%5" PRIu64 "", p->total);
    }

    snprintf(pref, sizeof pref, "%-6s/%6s", cur, tot);

    // We pad to max of 20 chars
    snprintf(buf, bsize, "%-20s [%s%s] %" PRIu64 "%%",
                pref, fill, blank, pct);
}


// update incremental progress
static void
update_incr_progress(char *buf, size_t bsize, progress *p)
{
    char cur[32];

    if (p->flags & P_HUMAN) {
        humanize_size(cur, sizeof cur, p->cur);
    } else {
        snprintf(cur, sizeof cur, "%" PRIu64 "", p->cur);
    }

    // We pad to max of 20 chars
    snprintf(buf, bsize, "%-20s ...", cur);
}

/* EOF */
