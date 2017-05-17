/* :vi:ts=4:sw=4:
 * 
 * xmalloc.h - Malloc with error checking.
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
 */

#ifndef __XMALLOC_H__
#define __XMALLOC_H__ 1

	/* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

#include <stddef.h>
    
#define xmalloc(x)      _xmalloc ((x), __FILE__, __LINE__)
#define xcalloc(n, s)   _xcalloc((n), (s), __FILE__, __LINE__)
#define xrealloc(e,n)   _xrealloc ((e), (n), __FILE__, __LINE__)
#define xfree(b)       _xfree (b, __FILE__, __LINE__)
    
extern void * _xmalloc (size_t nbytes, const char * file, unsigned int lineno) ;
extern void * _xcalloc (size_t nelem, size_t sizeof_each, const char * file, unsigned int lineno) ;
extern void * _xrealloc (void * existing, size_t nbytes, const char * file, unsigned int lineno) ;
extern void _xfree (void * buf, const char * file, unsigned int lineno) ;

extern void _xmalloc_init (const char * file);
extern void _xmalloc_fini (void);


#ifdef __cplusplus
}
#endif	/* __cplusplus */

#endif	/* ! __XMALLOC_H__ */

/* EOF */
