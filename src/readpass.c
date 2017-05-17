/*-
 * Copyright 2009 Colin Percival
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>

#include "utils/readpass.h"

#define MAXPASSLEN 2048
#define warnp(x) fprintf(stderr, x ": %s (%d)\n", strerror(errno), errno)

/**
 * portable_getpass(passwd, prompt, confirmprompt, devtty)
 * If ${devtty} is non-zero, read a password from /dev/tty if possible; if
 * not, read from stdin.  If reading from a tty (either /dev/tty or stdin),
 * disable echo and prompt the user by printing ${prompt} to stderr.  If
 * ${confirmprompt} is non-NULL, read a second password (prompting if a
 * terminal is being used) and repeat until the user enters the same password
 * twice.  Return the password as a malloced NUL-terminated string via
 * ${passwd}.  The obscure name is to avoid namespace collisions due to the
 * getpass / readpass / readpassphrase / etc. functions in various libraries.
 *
 * Returns:
 *     0   on success
 *     -EINVAL for too many wrong retries
 *    -errno on other failure
 */
int
portable_readpass(char ** passwd, const char * prompt,
                  const char * confirmprompt, int devtty)
{
    FILE * readfrom;
    char passbuf[MAXPASSLEN];
    char confpassbuf[MAXPASSLEN];
    struct termios term, term_old;
    int usingtty = 0,
        tries    = 3,
        r        = -1;
    size_t n     = 0;

    /*
     * If devtty != 0, try to open /dev/tty; if that fails, or if devtty
     * is zero, we'll read the password from stdin instead.
     */
    if (devtty == 0 || !(readfrom = fopen("/dev/tty", "r")))
        readfrom = stdin;

    if ((usingtty = isatty(fileno(readfrom))) != 0) {
        if (tcgetattr(fileno(readfrom), &term_old)) {
            r = -errno;
            warnp("Cannot read terminal settings");
            goto err1;
        }

        term = term_old;
        term.c_lflag = (term.c_lflag & ~ECHO) | ECHONL;
        if (tcsetattr(fileno(readfrom), TCSANOW, &term)) {
            r = -errno;
            warnp("Cannot set terminal settings");
            goto err1;
        }
    }

    do {
        if (usingtty) fprintf(stderr, "%s: ", prompt);

        if (fgets(passbuf, MAXPASSLEN, readfrom) == NULL) {
            r = -errno;
            warnp("Cannot read password");
            goto err2;
        }

        if (!confirmprompt) goto ok;

        if (usingtty) fprintf(stderr, "%s: ", confirmprompt);
        if (fgets(confpassbuf, MAXPASSLEN, readfrom) == NULL) {
            r = -errno;
            warnp("Cannot read password");
            goto err2;
        }
        if (0 == strcmp(passbuf, confpassbuf)) goto ok;

        if (--tries > 0) fprintf(stderr, "Passwords mismatch, please restart!\n");
    } while (tries > 0);

    r = -EINVAL;
    goto err2;


ok:
    n = strlen(passbuf);
    if (n > 0) {
        if (passbuf[n-1] == '\n')  passbuf[--n] = 0;
        if (passbuf[n-1] == '\r')  passbuf[--n] = 0;
    }


    if ((*passwd = strdup(passbuf)) == NULL) {
        r = -ENOMEM;
        goto err2;
    }

    memset(passbuf,     0, MAXPASSLEN);
    memset(confpassbuf, 0, MAXPASSLEN);
    r = 0;

err2:
    if (usingtty)          tcsetattr(fileno(readfrom), TCSAFLUSH, &term_old);

err1:
    if (readfrom != stdin) fclose(readfrom);

    return r;
}

