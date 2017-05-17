/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * cfg.c - simple config file parsing.
 *
 * Copyright (c) 2005 Sudhi Herle <sw at herle.net>
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

#include <errno.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "error.h"
#include "utils/cfg.h"
#include "utils/utils.h"
#include "utils/resolve.h"

// robust readline that handles CR, LF, CRLF as line break chars
static int
readline(FILE* fp, char * str, int n)
{
    char * end = str + n;

    while (1)
    {
        int c = fgetc(fp);
        if (c == EOF)
            return 0;

        if (ferror(fp))
            return -errno;

        if (c == '\r')
        {
            int next = fgetc(fp);
            if (next != EOF && next != '\n')
                ungetc(next, fp);

            break;
        }
        else if (c == '\n')
            break;

        if (str == end)
            return -ENOSPC;

        *str++ = c;
    }

    *str = 0;
    return str - (end - n);
}



int
cfg_file_open(cfg_file* cfg, const char* fn, int (*callback)(cfg_file*, int, char* v[2]))
{
    memset(cfg, 0, sizeof *cfg);

    cfg->fp = fopen(fn, "rb");
    if (!cfg->fp)
    {
        int err = errno;
        error(0, err, "Can't open config file '%s'", fn);
        return -err;
    }

    cfg->file     = fn;
    cfg->callback = callback;
    return 0;
}


int
cfg_file_close(cfg_file *cfg)
{
    if (cfg->fp)
        fclose(cfg->fp);

    return 0;
}


int
cfg_file_parse(cfg_file* cfg)
{
    char   str[256];
    char   line[1024];

    int linenum = 0;
    int err     = 0;
    size_t n    = 0;    // length of line we just read
    size_t m    = 0;    // indicates start of line when it is zero.


    line[0] = 0;
    while ( (n = readline(cfg->fp, str, sizeof str)) > 0 ) {
        char * p = strtrim(str);

        linenum++;

        //printf("%d:|%s|\n", linenum, str.c_str());

        if ( m > 0 ) {
            if ((m+n) >= sizeof line) {
                err  = -ENOSPC;
                error(0, 0, "%s:%d: Line too long\n", cfg->file, linenum);
                goto _err;
            }

            if (n > 0)
                memcpy(&line[m], p, n+1);
            m += n;
        } else {
            // start of a new line.
            if (0 == (n = strlen(p)))
                continue;

            memcpy(&line[0], str, n+1);
            m = n;
        }

        // continuation lines
        if (line[m-1] == '\\') {
            line[--m] = 0;
            continue;
        }

        // Skip comments
        if ( line[0] == '#' || line[0] == ';' ) {
            line[0] = 0;
            m = 0;
            continue;
        }


        //printf("full-%d:|%s|\n", linenum, line.c_str());


        // We have a full valid line to process
        {
            char * s    = &line[0];
            char * v[2] = { s, 0 };

            for (s = &line[0]; *s; s++) {
                if (isspace(*s)) {
                    *s   = 0;
                    v[1] = strtrim(s+1);
                    break;
                }
            }

            err = (*cfg->callback)(cfg, linenum, v);
            if (err < 0)
                goto _err;
        }
    }

    return 0;

_err:
    return err;
}


/*
 * Various parsers for different data types.
 *
 * All have the same signature; they return 0 on success, -Errno on
 * failure. The last param is cast to  a pointer of appropriate
 * type:
 *      - int:    long*
 *      - bool:   int*
 *      - size:   uint64_t*
 *      - string: char**
 *      - path:   char** (without ENV expansion)
 *
 */


int
cfg_parse_int(cfg_file* cfg, int linenum, char* str, void *val)
{
    uint64_t v = 0;
    int e = strtosize(str, 0, &v);

    if (e < 0) {
        error(0, 0, "%s:%d: Invalid integer value '%s'", cfg->file, linenum, str);
        return -EINVAL;
    }

    if (val)
        *(long *)val = (long)v;

    return 0;
}


int
cfg_parse_bool(cfg_file* cfg, int linenum, char* str, void *val)
{
    int v = (0 == strcasecmp("true", str) ||
             0 == strcasecmp("yes",  str) ||
             0 == strcasecmp("on",   str)) ? 1 : 0;
        
    if (val)
        *(int *)val = v;

    USEARG(cfg);
    USEARG(linenum);
    return 0;
}

int
cfg_parse_size(cfg_file* cfg, int linenum, char* str, void *val)
{
    uint64_t v = 0;
    int e = strtosize(str, 0, &v);

    if (e < 0) {
        error(0, 0, "%s:%d: Invalid integer value '%s'", cfg->file, linenum, str);
        return -EINVAL;
    }

    if (val)
        *(uint64_t *)val = v;

    return 0;
}



int
cfg_parse_string(cfg_file* cfg, int linenum, char* str, void *val)
{
    char * unq = 0;
    int r = strunquote(str, &unq);

    if (r < 0) {
        error(0, 0, "%s:%d: Missing ending quote char", cfg->file, linenum);
        return -EINVAL;
    }

    if (val)
        *(char **)val = strdup(unq);

    USEARG(cfg);
    USEARG(linenum);

    return 0;
}


int
cfg_parse_path(cfg_file* cfg, int linenum, char* str, void *val)
{
    char * unq = 0;
    int r = strunquote(str, &unq);

    if (r < 0) {
        error(0, 0, "%s:%d: Missing ending quote char", cfg->file, linenum);
        return -EINVAL;
    }

    if (val)
        *(char **)val = strdup(unq);

    USEARG(cfg);
    USEARG(linenum);

    /* XXX Path validation? ENV expansion? */
    return 0;
}

#if 0
int
cfg_parse_host_port(cfg_file* cfg, int linenum, char* str, void* val)
{
    char* v[2];
    int r = strsplit_quick(v, 2, str, ":", 1);
    char* host_s; char* port_s;
    int port;
    if_addr_vect* av = (if_addr_vect*)val;
    if_addr* z;

    if (r < 0)
    {
        error(0, 0, "%s:%d: Invalid HOST:PORT value '%s'", cfg->file, linenum, str);
        return -EINVAL;
    }

    host_s = strtrim(v[0]);
    port_s = strtrim(v[1]);

    if (0 == strlen(port_s) ||
        (port = atoi(port_s)) < 0 ||
        port > 65535)
    {
        error(0, 0, "%s:%d: Invalid PORT value '%s'", cfg->file, linenum, port_s);
        return -EINVAL;
    }


    if (resolve_host_or_ifname(host_s, av, 0) < 0)
    {
        error(0, 0, "%s:%d: Unable to resolve host '%s'", cfg->file, linenum, host_s);
        return -ENOENT;
    }

    VECT_FOR_EACH(av, z)
    {
        struct sockaddr_in *sin    = (struct sockaddr_in *)&z->sa;
        struct sockaddr_in6 * sin6 = (struct sockaddr_in6*)&z->sa;

        switch (z->sa.ss_family)
        {
            case AF_INET:
                sin->sin_port = htons(port);
                break;

            case AF_INET6:
                sin6->sin6_port = htons(port);
                break;
        }
    }

    return 0;
}
#endif


/* EOF */
