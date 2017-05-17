#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>

#include "error.h"
#include "utils/rotatefile.h"

using namespace std;

static int
fexists(const string& filename)
{
    struct stat st;

    int r = ::stat(filename.c_str(), &st);
    if (r < 0) {
        if (errno == ENOENT)
            return 0;

        error(1, errno, "Can't stat %s", filename.c_str());
    }

    return S_ISREG(st.st_mode);
}


int
main(int argc, const char* argv[])
{
    program_name = argv[0];

    if (argc < 3)
        error(1, 0, "Usage: %s file-to-rotate Keep", program_name);

    string fn(argv[1]);
    int  keep = atoi(argv[2]);

    if (keep <= 0) error(1, 0, "Invalid value for Keep: %s", argv[2]);


    printf("%s: Keeping %d recent files\n", fn.c_str(), keep);

    putils::rotate_filename(fn, keep, 0);

    // Now verify that we have removed other files.
    printf("Verifying older files ..\n");
    int i;
    for (i = keep; i < keep+100; ++i) {
        char suff[16];

        snprintf(suff, sizeof suff, ".%.d", i);
        string fexp(fn);

        fexp += suff;

        //printf("   %s ?\n", fexp.c_str());
        if (fexists(fexp)) error(1, 0, "** unexpected file %s", fexp.c_str());
    }
}

