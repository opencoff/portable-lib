/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * c_resolve.c - Resolution of interface names and addresses.
 *
 * Copyright (C) 2005, Sudhi Herle <sw at herle.net>
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
 * Creation date: Sun Sep 11 11:33:33 2005
 */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <ifaddrs.h>

#if defined(BSD)

    #include <net/if_dl.h>

    typedef struct sockaddr_dl  macaddr_type;
    // BSD already provides LLADDR()

#elif defined(__linux__)

    #include <linux/if_packet.h>

    #define AF_LINK AF_PACKET
    typedef struct sockaddr_ll  macaddr_type;
    #define LLADDR(dl)      (&(dl)->sll_addr[0])

#else
#error "Don't know how to build this file on this OS"
#endif

#include "utils/resolve.h"
#include "utils/utils.h"

struct iftmp
{
    if_addr ifa;
    char nm[IF_NAMESIZE];
};
typedef struct iftmp iftmp;

VECT_TYPEDEF(iftmp_vect, iftmp);


#if DEBUG
static void
hexdump(const char* prefix, const void * buf, size_t bufsiz)
{
    const char hex[]   = "0123456789abcdef";
    const uint8_t * p  = (const uint8_t*)buf;
    size_t rem         = bufsiz;
    char bb[128];

    printf("%s: %p %zu bytes\n", prefix, buf, bufsiz);

    while (rem > 0)
    {
        char * ptr   = &bb[0];
        char * ascii = ptr + ((8 * 2 + 7) * 2) + 4;
        size_t n = rem > 16 ? 16 : rem;
        const uint8_t * pe = p + n;
        int z = 0;

        rem -= n;
        memset(bb, ' ', sizeof bb);

        for (z = 0; p < pe; ++p, ++z)
        {
            unsigned char c = *p;
            *ptr++ = hex[c >> 4];
            *ptr++ = hex[c & 0xf];
            *ptr++ = ' ';
            if (isprint(c))
                *ascii++ = c;
            else
                *ascii++ = '.';

            if (z == 7)
            {
                *ptr++   = ' ';
                *ascii++ = ' ';
            }
        }
        *ascii = 0;

        printf("%p %s\n", p, bb);
    }
}
#endif



/*
 * Return true if 'name' is an interface name, false otherwise.
 */
static int
is_ifname(const char* name)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int r = 0;

    if (fd >= 0)
    {
        struct ifreq ifr;
        memset(&ifr, 0, sizeof ifr);

        strcopy(ifr.ifr_name, IFNAMSIZ, name);
        if (ioctl(fd, SIOCGIFMTU, &ifr) == 0)
            r = 1;

        close(fd);
    }
    //printf("ifname? %s => %d\n", name, r);
    return r;
}



const char*
sockaddr_to_string(char * buf, size_t bufsiz, const struct sockaddr * sa)
{
    const char hex[]         = "0123456789abcdef";
    struct sockaddr_in  * a4 = (struct sockaddr_in *)sa;
    struct sockaddr_in6 * a6 = (struct sockaddr_in6 *)sa;

    switch (sa->sa_family)
    {
        case AF_LINK:
        {
            macaddr_type * dl = (macaddr_type *)sa;
            unsigned char* m  = (unsigned char*)LLADDR(dl),
                         * me = m + 6;
            char *p = buf;

            //hexdump("macdump", sa, sizeof(struct sockaddr_storage));

            for (; m < me; ++m)
            {
                unsigned char c = *m;
                *p++ = hex[c >> 4];
                *p++ = hex[c & 0xf];
                *p++ = ':';
            }
            p[-1] = 0;

            return buf;
        }

        case AF_INET:
            return inet_ntop(sa->sa_family, &a4->sin_addr, buf, bufsiz);

        case AF_INET6:
            return inet_ntop(sa->sa_family, &a6->sin6_addr, buf, bufsiz);

        default:
            snprintf(buf, bufsiz, "unknown-AF %d", sa->sa_family);
            break;
    }
    return buf;
}



static int
iftmp_cmp(const void* a, const void* b)
{
    const iftmp* x = (const iftmp*)a;
    const iftmp* y = (const iftmp*)b;

    return strcmp(x->nm, y->nm);
}


int
get_all_if_address(if_address_vect* addrv, unsigned int mask)
{
    struct ifaddrs *ifa;
    struct ifaddrs *ifp;
    iftmp_vect av;
    int r;
    iftmp* prev;
    if_address * cur;

    VECT_INIT(&av, 8);

    VECT_RESET(addrv);
    VECT_RESERVE(addrv, 8);


    // By default, we only fetch IP addresses
    if (!mask)
        mask = F_INET|F_INET6;

    r = getifaddrs(&ifa);
    if (r < 0)
        return r;

    for (ifp = ifa; ifp; ifp = ifp->ifa_next)
    {
        iftmp t;
        if_addr * a = &t.ifa;
        size_t len  = 0;


        switch (ifp->ifa_addr->sa_family)
        {
            default:
            case AF_INET:
                if (!(mask & F_INET))
                    continue;
                break;

            case AF_INET6:
                if (!(mask & F_INET6))
                    continue;
                break;

            case AF_LINK:
                if (! (mask & F_LINK))
                    continue;
                break;

        }

        strcopy(t.nm, sizeof t.nm, ifp->ifa_name);

        /*
         * How ugly! Linux does not support sa_len field in struct sockaddr.
         */
#if defined(__linux__)
        switch (ifp->ifa_addr->sa_family)
        {
            default:
            case AF_INET:
                len = sizeof(struct sockaddr_in);
                break;
            case AF_INET6:
                len = sizeof(struct sockaddr_in6);
                break;
            case AF_LINK:
                len = sizeof(macaddr_type);
                break;

        }


#elif defined(BSD) /* Possibly more systems have sa_len */
        len = ifp->ifa_addr->sa_len;
#else
#error "I don't know how to build thie file on this OS"
#endif

        memcpy(&a->sa, ifp->ifa_addr, len);

        a->flags = ifp->ifa_flags;
        VECT_PUSH_BACK(&av, t);
    }

    freeifaddrs(ifa);

    /*
     * Sort the array 'av' and sort out duplicates
     */
    VECT_SORT(&av, iftmp_cmp);

    prev = &VECT_ELEM(&av, 0);
    cur  = &VECT_ELEM(addrv, 0);
    VECT_INIT(&cur->if_addr, 4);

    if (VECT_LEN(&av) == 0)
        return 0;

    strcopy(cur->if_name, sizeof cur->if_name, prev->nm);
    VECT_PUSH_BACK(&cur->if_addr, prev->ifa);


    size_t j;
    for (j = 1; j < VECT_LEN(&av); ++j)
    {
        iftmp* x = &VECT_ELEM(&av, j);

        if (0 != strcmp(prev->nm, x->nm))
        {
            VECT_ENSURE(addrv, 1);

            addrv->size += 1;

            cur = &VECT_ELEM(addrv, addrv->size);
            VECT_INIT(&cur->if_addr, 4);
            strcopy(cur->if_name, sizeof cur->if_name, x->nm);
        }

        VECT_PUSH_BACK(&cur->if_addr, x->ifa);
        prev = x;
    }

    addrv->size += 1;

#if 0
    VECT_FOR_EACH(addrv, cur)
    {
        char buf[64];
        if_addr* z;

        printf("%s: %d addresses:\n", cur->if_name, VECT_LEN(&cur->if_addr));
        VECT_FOR_EACH(&cur->if_addr, z)
        {
            printf("   %s\n", sockaddr_to_string(buf, sizeof buf, (struct sockaddr*)&z->sa));
        }
    }
#endif

    return 0;
}




// Get all addresses bound to an interface
int
get_interface_address(const char* ifname, if_addr_vect* addrv, unsigned int mask)
{
    int n;
    size_t j;

    if_address_vect av;
    if (!is_ifname(ifname))
        return -ENOENT;


    VECT_INIT(&av, 8);

    n = get_all_if_address(&av, mask);
    if (n < 0)
        return n;

    VECT_RESET(addrv);
    VECT_RESERVE(addrv, 8);

    for (j = 0; j < VECT_LEN(&av); ++j)
    {
        if_address* a = &VECT_ELEM(&av, j);

        if (0 == strcmp(a->if_name, ifname))
        {
            VECT_COPY(addrv, &a->if_addr);

#if 0
            printf("Found %s:\n", a->if_name);
            {
                if_addr* z;
                char buf[64];
                VECT_FOR_EACH(addrv, z)
                {
                    printf("   %s\n", sockaddr_to_string(buf, sizeof buf, (struct sockaddr*)&z->sa));
                }
            }
#endif
            break;
        }
    }

    return 0;
}



// Resolve a name (maybe interface name or hostname) into its
// addresses
int
resolve_host_or_ifname(const char* name, if_addr_vect* addrv, unsigned int mask)
{
    union un
    {
        struct sockaddr_in sin;
        struct sockaddr_storage    sa;
    } u;


    memset(&u, 0, sizeof u);
    u.sin.sin_family = AF_INET;

    VECT_RESET(addrv);
    VECT_RESERVE(addrv, 8);

    // Degenerate cases
    if (0 == strlen(name) || *name == '*')
    {
        if_addr x;
        memset(&x, 0, sizeof x);
        u.sin.sin_addr.s_addr = INADDR_ANY;
        x.sa = u.sa;

        VECT_PUSH_BACK(addrv, x);

        return 0;
    }
    else
    {
        struct in_addr a;


        if (inet_aton(name, &a))
        {
            if_addr x;
            memset(&x, 0, sizeof x);
            u.sin.sin_addr = a;
            x.sa = u.sa;
            VECT_PUSH_BACK(addrv, x);

            return 0;
        }
    }

    // if this is an interface name, we have a short-cut.
    if (get_interface_address(name, addrv, mask) == 0)
        return 0;


    /*
     * Think of this as a hostname and resolve it.
     *
     * XXX This is a synchronous, blocking call.
     */
    struct addrinfo hints;
    struct addrinfo* ai = 0;
    struct addrinfo* ptr;
    int n;

    if (!mask)
        mask = F_INET|F_INET6;

    memset(&hints, 0, sizeof hints);
    hints.ai_family   = 0;
    hints.ai_socktype = SOCK_STREAM;

    n = getaddrinfo(name, 0, &hints, &ai);
    if (n != 0)
        return -errno;

    for (ptr = ai; ptr; ptr = ptr->ai_next)
    {
        struct sockaddr* s = ptr->ai_addr;
        if_addr x;

        switch (s->sa_family)
        {
            default:
            case AF_INET:
                if (!(mask & F_INET))
                    continue;
                break;

            case AF_INET6:
                if (!(mask & F_INET6))
                    continue;
                break;

            case AF_LINK:
                if (! (mask & F_LINK))
                    continue;
                break;

        }

        memset(&x, 0, sizeof x);
        memcpy(&x.sa, s, ptr->ai_addrlen);

        VECT_PUSH_BACK(addrv, x);
    }

    freeaddrinfo(ai);
    return 0;
}


