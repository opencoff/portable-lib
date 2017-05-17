/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * darwin_cpu.c - CPU affinity API for OS X (darwin)
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

#include "utils/cpu.h"
#include <sys/types.h>
#include <sys/sysctl.h>
#include <errno.h>
#include "error.h"

int
sys_cpu_getavail(void)
{
    size_t old = 1;
    size_t oldlen = sizeof old;

    if (sysctlbyname("hw.logicalcpu", &old, &oldlen, 0, 0) < 0)
        error(1, errno, "Can't query sysctl('hw.logicalcpu')");

    return old;
}



int
sys_cpu_set_process_affinity(int cpu)
{
    (void)cpu;
    return 0;
}


void
sys_cpu_set_thread_affinity(pthread_t id, int cpu)
{
    (void)id;
    (void)cpu;
}


void
sys_cpu_set_my_thread_affinity(int cpu)
{
    (void)cpu;
}
/* EOF */
