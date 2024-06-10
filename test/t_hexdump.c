// test harness for hexdump

#include <assert.h>
#include <unistd.h>
#include <fcntl.h>

#include "utils/utils.h"

static void
hexlify(const char *fn)
{
    int fd = open(fn, O_RDONLY);
    assert(fd >= 0);

    uint8_t buf[4096];

    while (1) {
        ssize_t x = read(fd, buf, sizeof buf);
        assert(x >= 0);

        if (x == 0) break;

        int r = fhexdump(stdout, buf, x);
        assert(r == 0);
    }

    close(fd);
}

int
main(int argc, char *argv[])
{
    if (argc > 1) {
        int i = 0;
        for (i = 1; i < argc; i++) {
            hexlify(argv[i]);
        }
    } else {
        int fd = open("/dev/urandom", O_RDONLY);
        uint8_t buf[20];

        assert(fd >= 0);
        ssize_t x = read(fd, buf, sizeof buf);
        assert(x == sizeof buf);
        close(fd);

        size_t i = 0;
        for (i = 1; i <= sizeof buf; i++) {
            int r = 0;
            fprintf(stdout, "dump: %p %zd bytes\n", buf, i);
            r = fhexdump(stdout, buf, i);
            assert(r == 0);
            fprintf(stdout, "---\n");
        }
    }
}


// vim: ts=4:sw=4:expandtab:
