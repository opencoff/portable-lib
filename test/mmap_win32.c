/*
 * Test harness for win32 implementation of mmap()
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "error.h"

#include "utils/utils.h"

static void write_buf(void*, size_t);
static void read_file(int fd, void*, size_t);
static void write_file(int fd, void*, size_t);

int
main(int argc, char* argv[])
{
    char * buf = NEWZA(char, 65536);
    int fd;
    char* filename;
    void* addr;
    struct stat st;
    size_t mapsize = 65536;

    program_name = argv[0];

    if (argc < 2)
        error(1, 0, "Usage: %s filename", program_name);

    filename = argv[1];

    fd = open(filename, O_RDWR|O_BINARY);
    if (fd < 0)
        error(1, errno, "Can't open filename '%s'", filename);

    if (stat(filename, &st) < 0)
        error(1, errno, "Can't query file '%s'", filename);

    if ((size_t)st.st_size < mapsize)
        mapsize = st.st_size;

    addr = mmap(0, mapsize, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED)
        error(1, errno, "Can't mmap '%s'", filename);

    printf("Mapped %d bytes from %s at %p\n", mapsize, filename, addr);
    read_file(fd, buf, mapsize);
    if (0 != memcmp(buf, addr, mapsize))
        error(1, 0, "bufread and mmap don't compare");

    printf("Writing to file via fd ..\n");
    write_file(fd, buf, mapsize);
    if (0 != memcmp(buf, addr, mapsize))
        error(1, 0, "bufwrite and mmap don't compare");

    printf("Writing to file via mmap'd ptr..\n");
    write_buf(addr, mapsize);
    msync(addr, mapsize, 0);

    printf("Reading back from file via fd ..\n");
    read_file(fd, buf, mapsize);
    if (0 != memcmp(buf, addr, mapsize))
        error(1, 0, "bufwrite+bufread and mmap don't compare");

    munmap(addr, mapsize);
    close(fd);
    return 0;
}

static void
read_file(int fd, void* buf, size_t len)
{
    unsigned char* ptr = (unsigned char*)buf;

    memset(buf, 0, len);
    lseek(fd, 0, SEEK_SET);
    while (len > 0)
    {
        int n = read(fd, ptr, len);
        if (n < 0)
            error(1, errno, "Can't read from fd-%d", fd);
        else if (n == 0)
            error(1, errno, "Whoa; zero-byte read from fd-%d", fd);

        ptr += n;
        len -= n;
    }
}

void
write_buf(void* buf, size_t n)
{
    unsigned char* ptr = (unsigned char*)buf;
    uint32_t c = 0;

    while(n > 0)
    {
        *ptr++ = c++;
        if (c >= 255)
            c = 0;
        n--;
    }
}


static void
write_file(int fd, void* buf, size_t len)
{
    unsigned char* ptr = (unsigned char*)buf;

    write_buf(buf, len);

    lseek(fd, 0, SEEK_SET);
    while (len > 0)
    {
        int n = write(fd, ptr, len);
        if (n < 0)
            error(1, errno, "Can't write to fd-%d", fd);

        ptr += n;
        len -= n;
    }

    fsync(fd);
}

