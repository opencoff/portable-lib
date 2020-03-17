/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * resolve.h - Generic name to address resolver interface
 *
 * Copyright (c) 2010 Sudhi Herle <sw at herle.net>
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
 * Creation date: Aug 22, 2010 21:38:20 CDT
 */

#ifndef ___RESOLVE_H_3766274_1282531100__
#define ___RESOLVE_H_3766274_1282531100__ 1

#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>

#include "fast/vect.h"

#ifndef IF_NAMESIZE
#warn __FILE__ ": No definition for IF_NAMESIZE. Assuming 64"
#define IF_NAMESIZE   64
#endif

#ifdef __cplusplus
extern "C" {
#endif


struct if_addr
{
    struct sockaddr_storage sa;
    unsigned int flags;
};
typedef struct if_addr if_addr;

VECT_TYPEDEF(if_addr_vect, if_addr);

struct if_address
{
    if_addr_vect     if_addr;
    char if_name[IF_NAMESIZE];
};
typedef struct if_address if_address;

VECT_TYPEDEF(if_address_vect, if_address);

/*
 * Filtering masks
 */
#define     F_INET      (1 << 0)
#define     F_INET6     (1 << 1)
#define     F_LINK      (1 << 2)


/**
 * Get all interfaces and their addresses.
 *
 * Returns:
 *   0 on success
 *   -errno  on failure
 */
extern int get_all_if_address(if_address_vect*, unsigned int mask);


/*
 * Free all interface addresses
 */
extern void free_all_if_address(if_address_vect*);


/**
 * Get all addresses bound to an interface.
 *
 * Returns:
 *   0 on success
 *   -errno  on failure
 */
extern int get_interface_address(const char* ifname, if_addr_vect*, unsigned int mask);


/**
 * Resolve a name into a vector of addresses. Name can be an
 * interface name or a hostname.
 *
 * Returns:
 *   0 on success
 *   -errno  on failure
 */
extern int resolve_host_or_ifname(const char* name, if_addr_vect*, unsigned int mask);



/**
 * Convert a sockaddr to a string suitable for printing
 */
extern const char* sockaddr_to_string(char * buf, size_t bufsize, const struct sockaddr*);


#ifdef __cplusplus
}
#endif



#endif /* ! ___RESOLVE_H_3766274_1282531100__ */

/* EOF */
