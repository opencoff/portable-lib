/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * openbsd_cpu.c - CPU affinity API for OpenBSD
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

#include <unistd.h>
#include <errno.h>

#include "utils/cpu.h"

/*
 * Bizarre OS OpenBSD.
 *
 * It returns number of CPUs, but no way to set CPU affinity.
 */

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
    cpu ^= cpu;
    return 0;
}


void
sys_cpu_set_thread_affinity(pthread_t id, int cpu)
{
    cpu ^= cpu;
    id = id;
}


void
sys_cpu_set_my_thread_affinity(int cpu)
{
    cpu ^= cpu;
}
/* EOF */
