/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * t_bits.cpp - simple test harness for bit manip templates
 *
 * Copyright (c) 2005 Sudhi Herle <sw@herle.net>
 *
 * Licensing Terms: (See LICENSE.txt for details). In short:
 *    1. Free for use in non-commercial and open-source projects.
 *    2. If you wish to use this software in commercial projects,
 *       please contact me for licensing terms.
 *    3. By using this software in commercial OR non-commercial
 *       projects, you agree to send me any and all changes, bug-fixes
 *       or enhancements you make to this software.
 * 
 * This software does not come with any express or implied
 * warranty; it is provided "as is". No claim  is made to its
 * suitability for any purpose.
 *
 * Creation date: Thu Oct 20 10:58:22 2005
 */


#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include "utils/strutils.h"
#include "utils/utils.h"

using namespace putils;
using namespace std;


int
main(int argc, char* argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        char* str = argv[i];
        pair<bool, unsigned long long> p = strtoi<unsigned long long>(str);
        if (!p.first)
        {
            printf("Invalid number %s; ignoring ..\n", str);
            continue;
        }
        uint32_t ul = p.second & 0xffffffff;
        uint32_t up = align_up<uint32_t>(ul, 8192);
        uint32_t dn = align_down<uint32_t>(ul, 8192);
        uint64_t pow2 = round_up_pow2<uint64_t>(ul);
        bool     isok = is_aligned<uint32_t>(ul, 8192);

        printf("%s: <%saligned> @+8k: %u, @-8k: %u, @pow2: %" PRIu64 "\n",
                str, isok ? "" : "not ", up, dn, pow2);
    }
    return 0;
}
