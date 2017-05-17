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

#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
//#include <type_traits>
#include <vector>
#include <string>

namespace putils {


/**
 * Trim whitespaces from both sides of the string 'str'.
 *
 * @return The modified string with whitespaces removed.
 */
std::string string_trim(const std::string& str);



/**
 * Split a string 'str' into constituent parts delimited by one or more
 * characters in 'delim'. If 'sqz' is false, then consecutive
 * delimiter characters are considered to delimit separate fields.
 *
 * @return vector of strings separated by the delimiter.
 */
std::vector<std::string> string_split(const std::string& str,
                                  const std::string &delim, bool sqz=true, int maxsplits=-1);

/**
 * Split a comma separated string 'str' into constituent parts.
 *
 * @return vector of strings separated by ','
 */
std::vector<std::string> string_split_csv(const std::string& str);



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



/**
 * Convert string to lowercase
 */
inline std::string string_tolower(const std::string& s)
{
    std::string x(s);
    for(size_t i = 0; i < x.length(); ++i)
        x[i] = char(tolower(x[i]));

    return x;
}



/**
 * Convert string to lowercase
 */
inline std::string string_toupper(const std::string& s)
{
    std::string x(s);
    for(size_t i = 0; i < x.length(); ++i)
        x[i] = char(toupper(x[i]));

    return x;
}



/**
 * Convert string 's' to an equivalent boolean.
 * i.e., map "true", "yes", "1" to true
 *       others to false
 */
inline bool string_to_bool(const std::string& s)
{
    std::string x(string_tolower(s));
    return s == "yes" || s == "true" || s == "1" ? true : false;
}


/**
 * Convert an integer represented in string 's' into its value.
 * 
 * @param s     string to be converted into integer
 * @param base  force the use of a particular base for conversion
 *
 * @return  std::pair<bool, T> - a pair of values -- boolean status
 *          indicating success/failure of conversion and the value if
 *          successful.
 */
template<typename T> std::pair<bool, T> strtoi(const std::string& s, int base=0)
{
    std::pair<bool, T> ret (false, 0);
    const char * str = s.c_str ();
    char * endptr    = 0;

#ifdef _MSC_VER
// MS is weird. They deliberately choose NOT to use function names that
// the rest of the world uses.
#define strtoull(a,b,c)  _strtoui64(a,b,c)
#define _ULLCONST(n) n##ui64
#else

#define _ULLCONST(n) n##ULL

#endif
#define UL_MAX__   _ULLCONST(18446744073709551615)

    uint64_t v    = strtoull(str, &endptr, base);

    if (endptr && *endptr)
        return ret;

    if (v == UL_MAX__ && errno == ERANGE)
        return ret;

    ret.first  = true;
    ret.second = T(v);
    return ret;
}

/**
 * Convert an size represented in string 's' into its value.
 * The function can grok the following suffixes:
 *  - k,K: kilo bytes (1024)
 *  - M:   Mega bytes
 *  - G:   Giga bytes
 *  - T:   Tera bytes
 *  - P:   Peta bytes
 * 
 * @param s     string to be converted into integer
 * @param base  force the use of a particular base for conversion
 *
 * @return  std::pair<bool, T> - a pair of values -- boolean status
 *          indicating success/failure of conversion and the value if
 *          successful.
 */
inline std::pair<bool, uint64_t> strsizetoi(const std::string& s, int base=0)
{
    std::pair<bool, uint64_t> ret(false, 0);
    const char * str = s.c_str();
    char * endptr    = 0;

#define KB_  _ULLCONST(1024)
#define MB_  (KB_ * 1024)
#define GB_  (MB_ * 1024)
#define TB_  (GB_ * 1024)
#define PB_  (TB_ * 1024)


    uint64_t v    = strtoull(str, &endptr, base);
    uint64_t mult = 1;

    if ( endptr && *endptr )
    {
        switch (*endptr)
        {
            case 'k': case 'K':
                mult = KB_;
                break;

            case 'M':
                mult = MB_;
                break;

            case 'G':
                mult = GB_;
                break;

            case 'T':
                mult = TB_;
                break;

            case 'P':
                mult = PB_;
                break;

            default:
                return ret;
        }
    }

    if ( v == UL_MAX__ && errno == ERANGE )
        return ret;

    ret.first  = true;
    ret.second = uint64_t(v * mult);
    return ret;
}

} // namespace putils

#endif /* ! __STRUTILS_H_1111690568__ */

/* EOF */
