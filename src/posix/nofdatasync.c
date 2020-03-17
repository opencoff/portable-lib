/*
 * Dummy/Empty implementation of fdatasync for platforms that don't
 * have it.
 */

#include <unistd.h>
int
fdatasync(int fd)
{
    return fsync(fd);
}
