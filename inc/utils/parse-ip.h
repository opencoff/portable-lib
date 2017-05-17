/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * parse-ip.h - Simple IP address parsers
 *
 * Copyright (c) 2015 Sudhi Herle <sw at herle.net>
 *
 * Licensing Terms: GPLv2 
 *
 * If you need a commercial license for this work, please contact
 * the author.
 *
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 */

#ifndef ___PARSE_IP_H_8076929_1439847770__
#define ___PARSE_IP_H_8076929_1439847770__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>

/** parse a dotted-quad IPv4 address.
 *
 * Return:
 *    True  on success
 *    False on failure
 */
int parse_ipv4(uint32_t* p_z, const char* s);


/** Parse an IPv4 addr/mask combination.
 * -  a.b.c.d/nn
 * -  a.b.c.d/p.q.r.s
 *
 * Return:
 *    True  on success
 *    False on failure
 */
int parse_ipv4_and_mask(uint32_t* p_a, uint32_t* p_m, const char* str);


/*
 * convert address 'a' to dotted IP address.
 */
char* str_ipv4(char* dest, size_t sz, uint32_t a);


/*
 * Convert a mask to CIDR.
 *
 * Return:
 *   True on success and set '*cidr' to the converted mask
 *   False on failure
 */
int mask_to_cidr4(uint32_t mask, uint32_t* cidr);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! ___PARSE_IP_H_8076929_1439847770__ */

/* EOF */
