/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * daemon.h - Daemonize a process.
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
 *
 * Creation date: Dec 14, 2010 05:52:03 CST
 */

#ifndef ___DAEMON_H_2806318_1292327523__
#define ___DAEMON_H_2806318_1292327523__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern int daemonize(const char* pwd);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___DAEMON_H_2806318_1292327523__ */

/* EOF */
