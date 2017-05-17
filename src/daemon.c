/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * daemon.c - Common boilerplate to turn a POSIX process into a
 *            daemon
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
 * Creation date: Dec 14, 2010 05:51:40 CST
 */


#include "utils/daemon.h"

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "syserror.h"


int
daemonize(const char* pwd)
{
    pid_t pid = fork();

    if (pid < 0)
        return -geterrno();
    else if (pid > 0)
        exit(0);      // parent

    setsid();
    pid = fork();
    if (pid < 0)
        return -geterrno();
    else if (pid > 0)
        exit(0);

    // this is now the grandkid of the original process
    if (!pwd)
        pwd = "/tmp";

    chdir(pwd);
    umask(022);

    int fd0 = open("/dev/null", O_RDONLY);
    int fd1 = open("/dev/null", O_WRONLY);
    int fd2 = open("/dev/null", O_WRONLY);

    dup2(fd0, 0);
    dup2(fd1, 1);
    dup2(fd2, 2);

    return 0;
}

/* EOF */
