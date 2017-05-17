/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * utils/cpu.h - CPU Affinity API
 *
 * Copyright (c) 2011 Sudhi Herle <sw at herle.net>
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

#ifndef ___UTILS_CPU_H_8097479_1311065237__
#define ___UTILS_CPU_H_8097479_1311065237__ 1

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>


/**
 * Return number of CPUs that are online and available.
 */
extern int sys_cpu_getavail(void);


/* 
 * Bind current process to CPU 'cpu'.
 *
 * Note: CPUs are counted from 0.
 */
extern int sys_cpu_set_process_affinity(int cpu);

/* 
 * Set CPU affinity for thread 'thr' to 'cpu'.
 *
 * Note: CPUs are counted from 0.
 */
extern void sys_cpu_set_thread_affinity(pthread_t thr, int cpu);


/* 
 * Set CPU affinity for caller to 'cpu'
 *
 * Note: CPUs are counted from 0.
 */
extern void sys_cpu_set_my_thread_affinity(int cpu);

#ifdef __cplusplus
}
#endif

#endif /* ! ___UTILS_CPU_H_8097479_1311065237__ */

/* EOF */
