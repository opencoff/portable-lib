// test harness for hexdump

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>

#include "utils/hexdump.h"

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


static int
fp_writer(void *ctx, const char *buf, size_t n)
{
    FILE *fp = ctx;
    size_t m = fwrite(buf, n, 1, fp);
    if (m != 1) {
        return feof(fp) ? -EOF : ferror(fp);
    }
    return 0;
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
        for (i = 10; i <= sizeof buf; i++) {
            int r = 0;
            fprintf(stdout, "%p %zd bytes\n", buf, i);
            r = fhexdump(stdout, buf, i);
            assert(r == 0);
            fprintf(stdout, "---\n");
        }

        // now write one more time using ptrs instead of offsets
        hex_dumper d;

        hex_dump_init(&d, fp_writer, stdout, HEX_DUMP_PTR);
        fprintf(stdout, "%p %zd bytes\n", buf, sizeof buf);

        int r = hex_dump_write(&d, buf, sizeof buf);
        assert(r == 0);
        hex_dump_close(&d);
        fprintf(stdout, "---\n");
    }
}


// vim: ts=4:sw=4:expandtab:
