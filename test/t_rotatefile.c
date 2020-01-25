#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>

#include "error.h"
#include "utils/rotatefile.h"

static int
fexists(const char* filename)
{
    struct stat st;

    int r = stat(filename, &st);
    if (r < 0) {
        if (errno == ENOENT)
            return 0;

        error(1, errno, "Can't stat %s", filename);
    }

    return S_ISREG(st.st_mode);
}


int
main(int argc, const char* argv[])
{
    program_name = argv[0];

    if (argc < 3)
        error(1, 0, "Usage: %s file-to-rotate Keep", program_name);

    const char *fn = argv[1];
    int  keep = atoi(argv[2]);

    if (keep <= 0) error(1, 0, "Invalid value for Keep: %s", argv[2]);

    printf("%s: Keeping %d recent files\n", fn, keep);

    rotate_filename(fn, keep, 0);

    // Now verify that we have removed other files.
    printf("Verifying older files ..\n");
    int i;
    char fexp[PATH_MAX];
    for (i = keep; i < keep+100; ++i) {
        snprintf(fexp, sizeof fexp, "%s.%.d", fn, i);

        //printf("   %s ?\n", fexp.c_str());
        if (fexists(fexp)) error(1, 0, "** unexpected file %s", fexp);
    }
}

