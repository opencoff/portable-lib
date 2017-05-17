/* Simple test file for win32 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "error.h"


#include "utils/mmap.h"
#include "utils/utils.h"
#include <string>

#define xassert(e) do { \
        if (!(e)) { \
            printf("%s:%d: assertion '%s' failed\n", __FILE__, __LINE__, #e); \
            abort(); \
        }\
    } while(0)

#ifndef _WIN32
#include <signal.h>
static void
sigbus (int)
{
    printf("SIGBUS!\n");
    fflush(stdout);
    exit(1);
}
#endif /* !_WIN32 */

using namespace std;
using namespace putils;

#define FILESIZE    65536

int
main (int argc, char * argv [])
{
    program_name = argv[0];

    if ( argc < 2 )
        error(1, 0, "Usage: %s filename", program_name);

#ifndef _WIN32
    signal (SIGBUS, sigbus);
#endif /* !_WIN32 */

    string file (argv[1]);


    mmap_file * mm = 0;
    void * ptr     = 0;
    try {
       mm  = new mmap_file(file, MMAP_RDWR|MMAP_CREATE);
       ptr = mm->mmap();
    }
    catch (const sys_exception& ex) {
        error(1, 0, "** Exception: %s", ex.what());
    }

    xassert(ptr);

    const off_t CHUNKSIZE = 1048576;
    off_t    sz  = mm->filesize();
    off_t    off = 0;

    while (sz > 0) {
        off_t cur = min(sz, CHUNKSIZE);
        void *  mmp = mm->mmap(off, cur);
        memset(mmp, 0x77, cur);

        off += cur;
        sz  -= cur;
    }

    memcpy(ptr, "HELLO", 5);


    delete mm;

    try {
       mm  = new mmap_file(file, MMAP_RDONLY);
       ptr = mm->mmap();
    }
    catch (const sys_exception& ex) {
        error(1, 0, "** Exception: %s", ex.what());
    }

    xassert(ptr);

    xassert(0 == memcmp(ptr, "HELLO", 5));
    delete mm;
    return 0;
}

