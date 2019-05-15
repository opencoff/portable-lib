/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * strutils.h - Versatile String Utils
 *
 * Copyright (c) 2005-2008 Sudhi Herle <sw at herle.net>
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

#ifndef __STRUTILS_H_1111690568__
#define __STRUTILS_H_1111690568__ 1

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Trim a string by removing all leading and trailing white-spaces.
 */
extern char* strtrim(char* s);




/**
 * Split a string delimited by "tokens" into its constituent parts.
 * Returns a newly allocated array of strings - each of which point
 * into the original string. It is the caller's responsibility to
 * free this new array.
 *
 * NB: 'str' - the input string needs to be WRITABLE. The delimiters
 *     are replaced by '\0'. i.e., the array of strings point to
 *     places inside 'str' where the substrings begin.
 */
extern char** strsplit(int *strv_size, char* str, const char* tok, int sqz_consec);


/**
 * Split a string delimited by "tokens" into its constituent parts.
 * Fills the array 'strv' with pointers into the input string 'str'.
 * Returns:
 *
 *      < 0: if the array size was insufficient to hold all the
 *           constituent parts
 *      >=0: Number of split tokens in the original string; this
 *           number is necessarily <= n.
 *
 * NB: 'str' - the input string needs to be WRITABLE. The delimiters
 *     are replaced by '\0'.
 */
extern int  strsplit_quick(char** strv, int n, char* str, const char* tok, int sqz_consec);


/**
 * Split a Comma separated string, handle quoted strings and
 * escaped quotes.
 *
 * The separator can be any of characters in 'sep'. A nil ptr
 * indicates the _default_ of ','.  If 'sep' contains more than
 * one character, then any of them can be a valid separator.
 * e.g., ",;" indicates that ',' or ';' can be a field separator.
 *
 * Fills the array 'strv' with pointers into the input string 'str'.
 * Returns:
 *
 *      < 0: -ENOSPC if array is too small to hold the delimited parts
 *      >=0: Number of split tokens in the original string; this
 *           number is necessarily <= n.
 *
 * NB: 'str' - the input string needs to be WRITABLE. The delimiters
 *     are replaced by '\0'.
 */
extern int  strsplit_csv(char** strv, int n, char* str, const char *sep);


/**
 * Split a string into tokens just as shell would; handle quoted
 * words as a single word. Uses whitespace (' ', '\t') as delimiter
 * tokens. Delimited tokens are stuffed in 'args'.
 *
 * Returns:
 *
 *    >= 0  number of parsed tokens
 *    < 0   on error:
 *          -ENOSPC if array is too small
 *          -EINVAL if ending quote is missing
 */
extern int strsplitargs(char **args, size_t n, char *s);


/**
 * Convert a string 's' to a uint64_t and detect overflows.
 * Set '*endptr' to the last unconverted character. The converted
 * uint64_t is in 'p_ret'. If 'base' is 0, then auto-detect the
 * conversion base; else use what is provided.
 *
 * Returns:
 *      0   No error
 *    < 0   errno
 *          -ERANGE if string is too long and overflows uint64_t
 *          -EINVAL if string has invalid characters for the given base
 *                  or if the base is invalid (> 36).
 */
extern int strtou64(const char *s, const char **endptr, int base, uint64_t *p_ret);


/**
 * Parse a string 'str' containing a size suffix. The suffix has the
 * following meaning:
 *      B       bytes
 *      k, K    Kilobyte (1024)
 *      M       Megabyte (1024 * K)
 *      G       Gigabyte (1024 * M)
 *      T       Terabyte (1024 * G)
 *      P       Petabyte (1024 * T)
 *      E       Exabyte  (1024 * E)
 *
 * If the size suffix is followed by a lower case 'b', then it is
 * the suffix multiplier is treated as "bits". e.g., "Kb" is parsed
 * as "Kilo bit". Similarly, an upper case 'B' is treated as "byte".
 * This is the default interepretation. e.g., "1000M" is interpreted as
 * "1000 Mega bytes".
 *
 * Returns the parsed string in 'p_val'
 *
 * Returns:
 *      == 0: If parsing is successful
 *      <  0: -Errno on error
 */
extern int strtosize(const char* str, int base, uint64_t* val);



/**
 * Remove leading & trailing quote chars from the string.
 * If 'p_ret' is non-Null, return the resulting substring in
 * '*p_ret'.
 *
 * Return:
 *    0     No quote found
 *    > 0   Quote char that was removed (either '"' or '\'')
 *    < 0   No ending quote found
 *
 * NB: This function expects the caller to have already trimmed
 *     leading/trailing white spaces.
 */
extern int strunquote(char* s, char **p_ret);


/**
 * Robust readline() that handles CR, LF, CRLF line break
 * characters.
 * Returns:
 *    > 0: Length of the string
 *    0:   EOF
 *    -ENOSPC: Input buffer too small
 *    < 0: -errno for all other errors
 */
extern int freadline(FILE* fp, unsigned char * str, int n);



/**
 * Print 'n' in human readable units (understands upto EB).
 * Returns:
 *    original output buffer 'buf'
 */
extern char* humanize_size(char * buf, size_t bufsize, uint64_t n);



/**
 * Safe strcopy():
 *   - doesn't write more than 'sz' bytes into 'dest'
 *   - null terminates at all times
 *
 * Returns:
 *   >= 0  number of bytes copied
 *   < 0   truncated
 */
extern ssize_t strcopy(char* dest, size_t sz, const char* src);


/**
 * Decode hex chars in string 'str' and put them into 'out'.
 *
 * Return:
 *    < 0   -EINVAL Invalid hex chars in string
 *          -ENOMEM If out is too small to hold decoded string
 *    > 0   Number of decoded bytes.
 */
extern ssize_t str2hex(uint8_t* out, size_t outlen, const char* str);


#ifdef __cplusplus
}
#endif // cplusplus


#ifdef __cplusplus

#include <vector>
#include <string>

namespace putils {

/**
 * Add escape sequences to characters in 'str' if they contain chars
 * in 'escset'. And, use 'esc_char' to denote the escape sequence.
 *
 * @return String with special characters escaped.
 */
std::string string_escape(const std::string& str, const std::string& escset,
                           char esc_char = '\\');



/**
 * Remove escape sequences in 'str'  -- i.e., "process" any
 * characters that are escaped by 'esc_char'.
 *
 * @return String with escape sequences removed (processed).
 */
std::string string_unescape(const std::string& str, char esc_char = '\\');


template<typename T> std::pair<T, bool> strtoi(const std::string& s, int base=0)
{
    uint64_t v = 0;
    int      r = strtosize(s.c_str(), base, &v);

    return std::pair<T, bool>{T(v), r == 0 ? true : false};
}

} // namespace putils

#endif // __cplusplus

#endif /* ! __STRUTILS_H_1111690568__ */

/* EOF */
