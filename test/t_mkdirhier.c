#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include "error.h"

#include "utils/mkdirhier.h"

int
main(int argc, char* argv[]) {

    program_name = argv[0];

    if (argc < 2)
        error(1, 0, "Usage: %s directory [directory ...]\n", program_name);


    int i, err = 0;
    for (i = 1; i < argc; ++i) {
        const char* dn = argv[i];
        int r = mkdirhier(dn, 0700);

        if (r < 0) {
            ++err;
            error(0, -r, "Can't create directory %s\n", dn);
        }
    }
    return err;
}


