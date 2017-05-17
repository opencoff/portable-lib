/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * linux_cpu.c - CPU affinity API for Linux
 *
 * Copyright (c) 2010 Sudhi Herle <sw at herle.net>
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

/*
 * Fukking GLIBC morons. Can't keep symbols defined on a linux machine
 * without having to define a bunch of symbols.
 */
#define _GNU_SOURCE 1
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <errno.h>

#include "utils/cpu.h"


int
sys_cpu_getavail(void)
{
    long ncpus = 1;
#ifdef _SC_NPROCESSORS_ONLN
    ncpus = sysconf(_SC_NPROCESSORS_ONLN);
#endif

    return ncpus;
}

int
sys_cpu_set_process_affinity(int cpu)
{
    int r;
    cpu_set_t cpus;

    CPU_ZERO(&cpus);
    CPU_SET(cpu, &cpus);

    r = sched_setaffinity(0, sizeof(cpus), &cpus);
    return r < 0 ? -errno : 0;
}



void
sys_cpu_set_thread_affinity(pthread_t id, int cpu)
{
    cpu_set_t cpus;

    CPU_ZERO(&cpus);
    CPU_SET(cpu, &cpus);

    pthread_setaffinity_np(id, sizeof(cpus), &cpus);
}


void
sys_cpu_set_my_thread_affinity(int cpu)
{
    sys_cpu_set_thread_affinity(pthread_self(), cpu);
}

/* EOF */
