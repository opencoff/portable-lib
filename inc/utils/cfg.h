/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * cfg.h - Simple config file parsing
 *
 * Copyright (c) 2013 Sudhi Herle <sw at herle.net>
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

#ifndef ___CFG_H_9866549_1359943396__
#define ___CFG_H_9866549_1359943396__ 1

#include <stdio.h>

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct cfg_file
{
    const char* file;
    FILE * fp;
    int (*callback)(struct cfg_file*, int linenum, char * v[2]);
    void * opaq;
};
typedef struct cfg_file cfg_file;


extern int cfg_file_open(cfg_file*, const char* fn, int (*callback)(cfg_file*, int, char* v[2]));
extern int cfg_file_parse(cfg_file*);
extern int cfg_file_close(cfg_file*);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___CFG_H_9866549_1359943396__ */

/* EOF */
