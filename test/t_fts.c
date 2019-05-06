#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <error.h>
#include <inttypes.h>
#include <sys/stat.h>
#include "fts.h"

int
main(int argc, char* argv[])
{
    char* def = ".";
    FTS* fts;
    FTSENT* ent;

    if (argc < 2)
        argv[argc++] = def;

    argv[argc++] = 0;
    
    fts = fts_open(argv, FTS_LOGICAL|FTS_COMFOLLOW, 0);
    if (!fts)
        error(1, errno, "Can't begin tree walk");

    while ((ent = fts_read(fts)))
    {
    //  printf("accpath %s fts_path %s fts_name %s [%#" PRIx64 "] type %s\n",
    //          ent->fts_accpath, ent->fts_path, ent->fts_name,
    //          ent->fts_number, ent->fts_info & (FTS_D|FTS_DP) ? "DIR" : "FILE");
    }

    fts_close(fts);

    return 0;
}
