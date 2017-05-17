/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * $NetBSD: getopt.h,v 1.7 2005/02/03 04:39:32 perry Exp $
 *
 * Portability changes (c) 2005 Sudhi Herle <sw at herle.net>
 *
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Dieter Baron and Thomas Klausner.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __GETOPT_H_1127065823__
#define __GETOPT_H_1127065823__ 1

    /* Provide C linkage for symbols declared here .. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/*
 * When an option takes an argument, 'optarg' is used to communicate
 * the value of such an argument.
 */
extern char *optarg;


/*
 * Holds the current option index that is being scanned. On the very
 * first entry to getopt_long(), this must be set to zero. After
 * getopt_long() returns, this hold the index of the first
 * non-option element in 'argv[]'.
 */
extern int optind;


/*
 * Set this to zero to inhibit error messages being printed by
 * getopt_long().
 */
extern int opterr;


/*
 * getopt_long() sets this to option chars that are unrecognized.
 */
extern int optopt;


/*
 * names for values of the 'has_arg' field of 'struct option'
 */
#define no_argument        0
#define required_argument  1
#define optional_argument  2


/*
 * Data structure used to communicate long options that must be
 * recognized by getopt_long().  getopt_long() takes an array of
 * 'struct option'. This array must be terminated by an element
 * containing a name which is NULL (0).
 */
struct option
{
    /* name of long option */
    const char *name;

    /*
     * one of no_argument, required_argument, and optional_argument:
     * whether option takes an argument
     */
    int has_arg;

    /* if not NULL, set *flag to val when option found */
    int *flag;

    /* if flag not NULL, value to set *flag to; else return value */
    int val;
};


/** Long and short option parsing routine.
 *
 * This function parses short and long options. The caller must
 * ensure that 'optind' and 'opterr' are set appropriately.
 *
 * @param argc      Number of arguments in the array 'argv'
 * @param argv      Argument array (usually comes from  main())
 * @param short_opt String containing short options
 * @param long_opt  Array of long options terminated by NULL(0) name
 * @param idx       Pointer to index which will be set to the index
 *                  in long_opt that is matched.
 *
 * @return short option that is recognized OR the value of the 'val'
 *         field of the corresponding long-option. Returns EOF at
 *         the end of processing all the options.
 */
int getopt_long(int argc, char * const * argv, const char * short_opt, const struct option * long_opt, int *idx);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ! __GETOPT_H_1127065823__ */

/* EOF */
