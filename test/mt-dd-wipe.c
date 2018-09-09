/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * mt-dd-wipe.c - Multi-threaded disk wipe program.
 *
 * A multi-threaded version of "dd if=/dev/urandom". Uses as many
 * threads as CPUs on your machine. Assumes that your I/O subsystem
 * can handle the load of multiple writers to the same disk.
 *
 * Usage: $0 [options] FILE|DISKNAME
 *
 * Copyright (c) 2015 Sudhi Herle <sw at herle.net>
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
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>


#include "error.h"
#include "utils/cpu.h"
#include "fast/syncq.h"
#include "utils/utils.h"

// Auto-generated headerfile
#include "dd-wipe-opt.h"

extern uint32_t arc4random_buf(void*, size_t);

extern uint64_t get_disksize(int fd, const char* fnam);

// mmap in 256 MB chunks
#define CHUNKSIZE       (256 * 1048576)

// Progress Queue size
#define QUEUESIZE       1024


/*
 * Holds info to send back to the main thread.
 * This is progress info - sent periodically.
 */
struct progress
{
    int cpu;        // thread# doing the job
    pthread_t id;   // thread id

    uint64_t done,  // # of blocks done
             total; // total # of blocks

};
typedef struct progress progress;


// Synchronized Queue (producer-consumer)
SYNCQ_TYPEDEF(progq, progress, QUEUESIZE);



// Per thread context
struct ctxt
{
    int         cpu;        // CPU# 
    int         fd;         // fd we are working with
    int         wipes;      // how many times to wipe each block
    uint32_t    iopause;    // # of milliseconds of waiting before starting next I/O
    uint32_t    pgsize;     // system page size

    uint64_t    iosize;

    pthread_t id;   // thread id
    progq*  q;      // queue for sending progress status

    uint8_t* iobuf; // IO Buf (for non-mmapable targets)

    uint64_t   start,  // starting offset
               count;  // count of bytes
};
typedef struct ctxt ctxt;


/*
 * Struct to measure progress bar of each CPU.
 */
struct speedo
{
    uint64_t *done;
    uint64_t *total;
    uint64_t start;
    uint64_t prev;

    int     ncpu;
};
typedef struct speedo speedo;

// Init progress bar
static void
speedo_init(speedo* sp, int ncpu)
{
    sp->done  = NEWZA(uint64_t, ncpu);
    sp->total = NEWZA(uint64_t, ncpu);
    sp->ncpu  = ncpu;

    sp->start = timenow();
    sp->prev  = 0;
}


// Show progress bar
static void
speedo_show(speedo* sp, int cpu, uint64_t done, uint64_t tot)
{
    uint64_t tm = timenow() - sp->start;

    sp->done[cpu]  = done;
    sp->total[cpu] = tot;

    if (tm < 100000) return;

    char buf[256];
    char* p = &buf[0];
    int i;
    ssize_t n,
            av = sizeof buf;

#define dd(x)       ((double)(x))
#define rate(a,b)   ((b) > 0 ? (100.0 * dd(a)) / dd(b) : 0.0)

    uint64_t td = 0;
    for (i = 0; i < sp->ncpu; ++i) {
        td += sp->done[i];
        n = snprintf(p, av, "%02d: %4.1f%% ", i, rate(sp->done[i], sp->total[i]));
        p  += n;
        av -= n;
    }

    //printf("total %llu, elapsed %llu us\n", td, tm);

    uint64_t z  = (td - sp->prev) / 1048576;    // MB
    double secs = dd(tm) * 1.0e-6;              // elapsed time in seconds
    snprintf(p, av, "%5.2f MB/s\r", dd(z) / secs);

    sp->prev  = td;
    sp->start = timenow();

    fputs(buf, stdout);
    fflush(stdout);
}


// write(2) that ignores EINTR
static void
fullwrite(int fd, uint8_t* buf, uint64_t sz, uint64_t off)
{
    while (sz > 0) {
        ssize_t n = pwrite(fd, buf, sz, off);

        if (n < 0) {
            if (errno == EINTR) continue;

            error(1, errno, "Write failed at offset %llu (%llu bytes)", off, sz);
        }

        sz  -= n;
        off += n;
        buf += n;
    }
}


/*
 * Darwin doesn't let us mmap a block device.
 */
#if defined(__APPLE__) // || defined(__linux__)

#define IOBUF_INIT(cx)  do {    \
            cx->iobuf = NEWA(uint8_t, cx->iosize); \
            if (!cx->iobuf)  \
                error(1, ENOMEM, "Memory exhausted while allocating %llu bytes", cx->iosize);\
        } while (0)

#define IOTHREAD_FUNC   copying_thread_func



// Uses write(2) to write random junk. 
// Some Posix systems don't allow you to mmap() block devices.
static void*
copying_thread_func(void* x)
{
    ctxt* cx = x;
    uint64_t n = cx->count;
    int i;
    progress pp;

    pp.cpu   = cx->cpu;
    pp.id    = cx->id;
    pp.total = cx->count;
    pp.done  = 0;

    while (n > 0) {
        uint64_t m  = n > cx->iosize ? cx->iosize : n;

        //printf("%d: chunk %llu off %llu; rem %llu\n", cx->cpu, m, cx->start, n);

        for (i = 0; i < cx->wipes; ++i) {
            arc4random_buf(cx->iobuf, m);
            fullwrite(cx->fd, cx->iobuf, m, cx->start);
        }

        n         -= m;
        cx->start += m;
        pp.done   += m;

        SYNCQ_ENQ(cx->q, pp);

        // Let the device catchup with the I/O
        if (cx->iopause) usleep(cx->iopause * 1000);
    }
    return 0;
}




#else

#define IOBUF_INIT(cx)  cx->iobuf = 0
#define IOTHREAD_FUNC   mmap_thread_func

// Use mmap(2) to write random junk at the given offset
static void*
mmap_thread_func(void* x)
{
    ctxt* cx = x;
    uint64_t n = cx->count;
    int i;
    progress pp;

    pp.cpu   = cx->cpu;
    pp.id    = cx->id;
    pp.total = cx->count;
    pp.done  = 0;

    while (n > 0) {
        uint64_t m  = n > cx->iosize ? cx->iosize : n;

        if (m < cx->pgsize) break;
        else if (!_IS_ALIGNED(m, cx->pgsize))
            m = cx->pgsize;

        void* mm = mmap(0, m, PROT_WRITE, MAP_SHARED, cx->fd, cx->start);
        if (MAP_FAILED == mm)
            error(1, errno, "CPU%d: Can't mmap %llu bytes at offset %llu",
                    cx->cpu, m, cx->start);

        for (i = 0; i < cx->wipes; ++i) {
            arc4random_buf(mm, m);
        }

        munmap(mm, m);

        n         -= m;
        cx->start += m;
        pp.done   += m;

        SYNCQ_ENQ(cx->q, pp);

        // Let the device catchup with the I/O
        if (cx->iopause) usleep(cx->iopause * 1000);
    }

    if (n > 0) {
        uint8_t rr[n];

        for (i = 0; i < cx->wipes; ++i) {
            arc4random_buf(rr, n);
            fullwrite(cx->fd, rr, n, cx->start);
        }
    }
    return 0;
}

#endif /* __APPLE__ */





/*
 * Start a new thread to handle 'count' bytes starting at 'st'.
 *
 * Return next offset to start.
 */
static uint64_t
start(ctxt* cx, int cpu, uint64_t st, uint64_t count)
{
    int r;
    int nmax  = sys_cpu_getavail();

    // Align the offset at 8k boundary (page size)

    cx->cpu   = cpu;
    cx->count = count;
    cx->start = st;

    printf("CPU %d: Off %llu size %llu\n", cpu, st, count);

    r = pthread_create(&cx->id, 0, IOTHREAD_FUNC, cx);
    if (r != 0) 
        error(1, r, "Can't create thread# %d", cx->cpu);

    if (cpu < nmax)
        sys_cpu_set_thread_affinity(cx->id, cpu);

    return st + count;
}



int
main(int argc, char **argv)
{
    opt_option opt;

    program_name = argv[0];

    if (opt_parse(&opt, argc, argv) != 0)
        exit(1);

    if (opt.argv_count <= 0)
        error(1, 0, "Usage: %s [options] device-name", program_name);

#ifndef O_DIRECT
#define O_DIRECT 0
#endif

    char* dev = opt.argv_inputs[0];
    int fd    = open(dev, O_RDWR|O_NONBLOCK|O_DIRECT);

    if (fd < 0)
        error(1, errno, "Can't open %s for writing", dev);

    uint32_t pgsize   = sysconf(_SC_PAGESIZE);
    uint64_t disksize = get_disksize(fd, dev);

    // We start at least _one_ thread
    if (opt.ncpu <= 0)
        opt.ncpu = sys_cpu_getavail();

    if (opt.iosize == 0)
        opt.iosize = 256 * 1048576;
    else if ((opt.iosize & ~(pgsize-1)) != opt.iosize)
        error(1, 0, "IO size %llu is not a multiple of system page size (%lu)", opt.iosize, pgsize);

    ctxt    cxs[opt.ncpu];
    progq   pq;
    speedo  sp;
    int i;


    i = SYNCQ_INIT(&pq, QUEUESIZE);
    if (i != 0) error(1, -i, "Can't initialize SyncQ");

    speedo_init(&sp, opt.ncpu);

    // ensure frac is a multiple of page size.
    uint64_t frac, rem;

    frac = disksize / opt.ncpu;
    frac = _ALIGN_DOWN(frac, pgsize);
    rem  = disksize - (frac * opt.ncpu);

    // Initialize every instance
    for (i = 0; i < opt.ncpu; ++i) {
        ctxt* cx = &cxs[i];

        memset(cx, 0, sizeof *cx);

        cx->fd      = fd;
        cx->wipes   = opt.wipes;
        cx->iosize  = opt.iosize;
        cx->iopause = opt.iopause;
        cx->pgsize  = pgsize;
        cx->q       = &pq;

        // Darwin doesn't let us mmap block devices. Grr.
        // So, we create a per-thread random buffer
        IOBUF_INIT(cx);
    }


    if (opt.verbose) {
        char b0[128];
        char b1[128];

        humanize_size(b0, sizeof b0, disksize);
        humanize_size(b1, sizeof b1, opt.iosize);
        printf("%s: %s; using %d threads to wipe %d time%s [IO size %s, IO pause %d ms]\n",
                dev, b0, opt.ncpu, opt.wipes, opt.wipes > 1 ? "s" : "", b1, opt.iopause);
    }

    // Start the first thread separately
    // We want to account for the fraction separately
    uint64_t st = 0;
    for (i = 0; i < opt.ncpu-1; ++i) {
        ctxt* cx = &cxs[i];

        st = start(cx, i, st, frac);
    }

    // Last thread does frac + rem
    start(&cxs[i], i, st, frac + rem);

    /*
     * Now, wait for the threads to complete and show progress
     * reports.
     */
    int done = opt.ncpu;
    while (done > 0) {
        progress p = { .cpu = 0, .id = 0, .done = 0 };

       SYNCQ_DEQ(&pq, p);

        if (p.done == p.total) {
            void* rr;

            done--;
            pthread_join(p.id, &rr);
        }
        if (opt.verbose) {
            speedo_show(&sp, p.cpu, p.done, p.total);
        }
    }

    if (opt.verbose) {
        fputc('\n', stdout);
    }

    return 0;

}


const char*
opt_usage()
{
    static char msg[1024];

    snprintf(msg, sizeof msg, "Usage: %s [options] device-name", program_name);
    return msg;
}

/* EOF */
