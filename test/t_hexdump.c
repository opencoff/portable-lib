// test harness for hexdump

#include <assert.h>
#include <unistd.h>
#include <fcntl.h>

#include "utils/utils.h"


int
main()
{
    int fd = open("/dev/urandom", O_RDONLY);
    uint8_t buf[64];

    assert(fd >= 0);
    ssize_t x = read(fd, buf, sizeof buf);
    assert(x == sizeof buf);
    close(fd);

    size_t i = 0;
    for (i = 1; i <= sizeof buf; i++) {
        fprintf(stdout, "dump: %p %zd bytes\n", buf, i);
        fhexdump(stdout, buf, i);
        fprintf(stdout, "---\n");
    }
}


