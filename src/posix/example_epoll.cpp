/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * example_epoll.cpp - Example epoll listener, listens on 100s of
 * ports and reflects I/O.
 *
 * Copyright (c) 2005 Sudhi Herle <sw at herle.net>
 *
 * Licensing Terms: GPLv2 
 *
 * If you need a commercial license for this work, please contact
 * the author.
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 *
 * Creation date: Thu Oct 20 11:05:35 2005
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <error.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <vector>
#include <map>

using namespace std;

extern "C" {

char* program_name = 0;

}

// map to track fds that are being listened to
typedef map<int, bool> listen_fds;


// simple template for dealing with resizable POD arrays
template <typename T>
struct quick_vect
{
    T* arr;
    int  size;
    int  cap;

    quick_vect()
        : size(0),
          cap(1024)
    {
        arr = (T *)calloc(cap, sizeof (T));
    }

    ~quick_vect()
    {
        free(arr);
    }

    T* operator[](int i) { return &arr[i]; }

    void add()
    {
        if (size >= cap)
        {
            int newsz = cap*2;
            T* newarr = (T*)realloc(arr, sizeof(T)*newsz);

            cap = newsz;
            arr = newarr;
        }
        ++size;
    }

    void del() { --size; }

    T * array() { return arr; }
};


// Data structure to keep track of listen fds, accepted fds etc.
// meant as a simple container of sorts.
struct listen_set
{
    listen_fds fds;
    quick_vect<struct epoll_event> pollable;
    int  epfd;

    listen_set() 
    {
        epfd = epoll_create(1024);
        if (epfd < 0)
            error(1, errno, "Can't create epoll FD");

    }

    void add(int fd)
    {
        _add(fd);
    }

    void add_listener(int fd)
    {
        _add(fd);
        fds[fd] = true;

        if (::listen(fd, 1024) < 0)
            error(1, errno, "Can't establish listener for fd %d", fd);
    }
    
    bool is_listener(int fd)
    {
        listen_fds::const_iterator i = fds.find(fd);
        return i != fds.end();
    }

    void _add(int fd)
    {
        struct epoll_event epev;

        epev.data.fd = fd;
        epev.events  = EPOLLIN|EPOLLOUT|EPOLLERR|EPOLLHUP|EPOLLET;
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &epev) < 0)
            error(1, errno, "Can't add fd %d to epoll set", fd);

        pollable.add();
    }

    void del(int fd)
    {
        struct epoll_event epev;

        epev.data.fd = fd;
        epev.events  = 0;
        if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &epev) < 0)
            error(1, errno, "Can't del fd %d from epoll set", fd);

        pollable.del();
    }

    size_t size()
    {
        return fds.size();
    }
};


// set fd to non blocking mode.
static void
set_nonblocking(int fd, bool set = true)
{
    int opt       = !!set;

    // XXX How many of these ought I fiddle with?

    int e = ::ioctl(fd, FIONBIO, &opt);
    if ( e < 0 )
        error(1, errno, "Can't set non-blocking flag for fd %d", fd);

    int flags = ::fcntl(fd, F_GETFL, 0);

    if ( set )
    {
        if (flags >= 0 && !(flags & O_NONBLOCK))
            flags = ::fcntl(fd, F_SETFL, flags | O_RDWR | O_NONBLOCK);
    }
    else
    {
        if (flags >= 0 && (flags & O_NONBLOCK))
            flags = ::fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
    }
    if ( flags < 0 )
        error(1, errno, "Can't set O_NONBLOCK for fd %d", fd);
}



// start N listeners to listen on IP 'a'.
static void
start_listeners(listen_set* s, struct in_addr* a, int start, int end)
{
    int i;

    for (i = start; i < end; i++)
    {
        int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (fd < 0)
            error(1, errno, "Can't create socket for port %d", i);


        struct sockaddr_in sin;

        sin.sin_family = AF_INET;
        sin.sin_addr   = *a;
        sin.sin_port   = htons(i);
        if (::bind(fd, (struct sockaddr*)&sin, sizeof sin) < 0)
            error(1, errno, "Unable to bind for port %d", i);
        
        //set_nonblocking(fd);
        s->add_listener(fd);
        printf("Added port %d to listen set: fd %d\n", i, fd);
    }
}

// Gather all FDs that are readable.
// Return the number of FDs that are ready
static int
gather_fds(listen_set* s)
{
    struct epoll_event* epev = s->pollable.array();
    int totfds = s->pollable.size;

    printf("Waiting for epoll: epev=%p, totfds=%d.. ", epev, totfds);
    fflush(stdout);

    int nfds = ::epoll_wait(s->epfd, epev, totfds, -1);
    if (nfds < 0)
        error(1, errno, "epoll wait failed!");

    printf(" %d fds are ready for I/O\n", nfds);
    return nfds;
}


// simplistic I/O handler that does Loopback. Looped back data has "R: " as the first 3 bytes.
static int
do_io(int fd, uint32_t flags)
{
    char buf[1024];
    int n = 0;

    if (flags & EPOLLIN)
    {
        n = read(fd, buf+3, (sizeof buf) - 3);
        if (n == 0)
            return 0;
    }

    if ( (flags & EPOLLOUT) && n > 0 )
    {
        buf[0] = 'R';
        buf[1] = ':';
        buf[2] = ' ';
        write(fd, buf, n+3);
    }

    return 1;
}


// dispatch fds that are ready
static void
dispatch_fds(listen_set* s, int nfds)
{

    for (int i = 0; i < nfds; ++i)
    {
        // we can't cache epev outside the array - since the s->add() method below might
        // end up realloc'ing the pollable array.
        struct epoll_event* epev = s->pollable[i];
        int fd = epev->data.fd;
        uint32_t flags = epev->events;
        printf("fd %d is ready for %s %s %s %s\n",
                fd, flags & EPOLLIN ? "IN" : "",
                    flags & EPOLLOUT ? "OUT" : "",
                    flags & EPOLLERR ? "ERR" :  "",
                    flags & EPOLLHUP ? "HUP" :  ""
                );


        // do I/O on non-listen sockets
        if (!s->is_listener(fd))
        {
            int r = do_io(fd, flags);
            if (r <= 0)
            {
                printf("Removing fd %d from epoll set\n", fd);
                s->del(fd);
            }

            continue;
        }

        struct sockaddr sa;
        struct sockaddr_in *sin = (struct sockaddr_in*)&sa;
        socklen_t slen = sizeof sa;
        int newfd = ::accept(fd, &sa, &slen);
        if (newfd < 0)
            error(1, errno, "Accept() on fd %d failed", fd);

        struct sockaddr this_sa;
        struct sockaddr_in *this_sin = (struct sockaddr_in*) &this_sa;
        slen = sizeof this_sa;

        if (getsockname(newfd, &this_sa, &slen) < 0)
            error(1, errno, "Can't find out local endpoint for new fd %d", newfd);

        char peer[64];
        char self[64];
        inet_ntop(AF_INET, &sin->sin_addr, peer, sizeof peer);
        inet_ntop(AF_INET, &this_sin->sin_addr, self, sizeof self);
        printf("New connection: %s:%d->%s:%d\n",
                peer, ntohs(sin->sin_port), self, ntohs(this_sin->sin_port));

        // Now, add this FD to global list
        set_nonblocking(newfd);
        s->add(newfd);
    }
}

static void
event_loop(listen_set* s)
{

    while(1)
    {
        int nfds = gather_fds(s);       
        dispatch_fds(s, nfds);
    }
}

int
main(int argc, char* argv[])
{
    listen_set all_fds;

    char* ip;
    struct in_addr a;
    int start,
        end;

    program_name = argv[0];

    if (argc < 4)
        error(1, 0, "Usage: %s LISTEN_IP START_PORT END_PORT",
                program_name);

    ip = argv[1];
    if (inet_pton(AF_INET, ip, &a) < 0)
        error(1, errno, "Can't decode IP %s", ip);

    start = atoi(argv[2]);
    end   = atoi(argv[3]);

    if ( start <= 0 || end <= 0 || start > 65534 || end > 65534 )
        error(1, 0, "Invalid Source/End ports %s? %s?",
                argv[2], argv[3]);

    if ( end < start )
        error(1, 0, "End port %d must be greater than start port %d",
                end, start);

    printf("Listening on %s:%d-%d [%d ports]\n",
            ip, start, end, end - start);

    start_listeners(&all_fds, &a, start, end);
    event_loop(&all_fds);

    return 0;
}
