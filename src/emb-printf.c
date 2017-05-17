/* :vi:ts=4:sw=4:
 *
 * emb-printf.c -- portable integer only printf() for embedded targets.
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

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#ifdef UNIT_TEST
#define _XPRINTF    Printf
#define _XSPRINTF   Sprintf
#else
#define _XPRINTF    printf
#define _XSPRINTF   sprintf
#endif /* UNIT_TEST */

/*
 * This subset of printf () supports the following conversion types:
 *
 *      %d         - output number as a signed decimal
 *      %u         - output number as a unsigned decimal
 *      %o         - output number as an unsigned octal number
 *      %x, %X, %p - output number as unsigned hexadecimal
 *      %c         - output a single character
 *      %s         - ouput a string
 *      %H         - output an IP address given a 32-bit host
 *                   byte order integer
 *      %N         - output an IP address given a 32-bit network
 *                   byte order integer
 *
 *   In addition, the normal width and precision specifiers may be
 *   used.
 *
 *   NOTE: According to ANSI-C, any arguments passed via '...' are
 *   automatically widened to  'int'
 */

/* -- Routines that implement _XPRINT(). -- */

/*
 * This structure is used to store information obtained from parsing a
 * format specifier.
 */
struct sfmt_info
{
    long width;             /* field width */
    long precision;         /* precision specifier */
    unsigned long count;    /* character count */
    
    /* Output routines. */
    void    * arg;
    void    (*outchar)(void *, char ch);

    /* BOOLEAN flags with default values */
    char space;           /* 0 */
    char right;           /* 1 */
    char alter;           /* 0 */
    char plus;            /* 0 */
    char zero;            /* 0 */
    char mod;             /* process modifier */
    char type;            /* conversion type */
};
typedef struct sfmt_info sfmt_info;

    /* Used to setup formatted output */
#define DEFAULT         -1
#define DEFAULT_P       6


/* External interfaces */

/* Output one character at a time. */
extern void _sys_charout(int ch);


/* -- Internal functions -- */
static void strout(sfmt_info * info, char * str, long len);
static void charout(sfmt_info * info, va_list *arg_pt);
static void stringout(sfmt_info *info, va_list *arg_pt);
static void decout(sfmt_info * info, int do_unsigned, va_list * arg_pt);
static void ipout(sfmt_info * info, int is_netorder, va_list * arg_pt);
static void hexout(sfmt_info *info, va_list *arg_pt);
static const char * percent(const char *format, sfmt_info *info, va_list *arg_pt);
static void _print_internal(sfmt_info * info, const char * format, va_list *arg_pt);


/* Write a character to stdout */
static void
putb(void * __x, char ch)
{
    _sys_charout(ch);
}

/* Write `ch' into `str' and advance str. */
static void
putstr(void * str, char ch)
{
    char ** x =(char **) str;
    *(*x)++ = ch;
}


/*
 * Output a formatted string, adding fill characters and field
 * justification. 
 * Input:
 *      info:    ptr to format data structure
 *      str:        ptr to string
 *      len:        length of string
 */
static void
strout(sfmt_info * info, char * str, long len)
{
    long size;
    unsigned long i;
    char pad = ' ';
    int c;

    /* Check for a NULL string. */
    if (len == 0)
        str = (char *)" (null)";

    /* Calc field width */
    size = (info->width < len ) ? len : info->width;

    /* Right justify ? */
    if (info->right)
    {
        for (i = size - len; i > 0; i--)
            info->outchar(&info->arg, pad);
        size = len;
    }

    /* print string until EOS or width is satisfied. */
    for (; (c = *str) && len > 0; str++, len--)
    {
        info->outchar(&info->arg, c);
        size--;
    }

    /* If left justified, it may be necessary to pad end of the
     * string. */
    for (i = size - len; i > 0; i--)
        info->outchar(&info->arg, pad);
}


/*
 * charout: Output a character
 *   INPUTS: info = ptr to format data structure
 *           arg_pt  = ptr to next argument in list
 */
static void
charout(sfmt_info * info, va_list *arg_pt)
{
    char cbuf[2];

    cbuf[0] = (unsigned long) va_arg((*arg_pt), unsigned long);
    cbuf[1] = 0;
    strout(info, cbuf, 1);
}


/*
 * Output a string
 *  INPUTS: info = ptr to format data structure
 *          arg_pt  = ptr to next argument in list
 *          
 * NOTE: Strings longer than MAX_STRING_SIZE bytes
 * are truncated!
 */
static void
stringout(sfmt_info *info, va_list *arg_pt)
{
    char * str = (char *) va_arg((*arg_pt), int);

    if ( !str )
        str = "(null)";

    strout(info, str, strlen(str));
}

static char*
write_u(char* p, unsigned long v)
{
    /* write from last to first */
    do
    {
         *(p--) = v % 10 + '0';
         v /= 10;

    } while (v);

    return p;
}

static char*
write_oct(char* p, unsigned int v)
{
    if (v > 99)
        p += 2;
    else if (v > 9)
        p += 1;

    write_u(p, v);
    return p + 1;
}

/*
 * Output a value is an IP address.
 */
static void
ipout(sfmt_info* info, int is_netorder, va_list* arg_pt)
{
    uint32_t  ip = (uint32_t) va_arg(*arg_pt, uint32_t);
    char strbuf[32], /* we know IPv4 addrs are no bigger than this */
        *p = strbuf;

    if (is_netorder)
    {
        uint32_t x0 = (ip >> 24)  & 0xff;
        uint32_t x1 = (ip >> 16)  & 0xff;
        uint32_t x2 = (ip >>  8)  & 0xff;
        uint32_t x3 = (ip >>  0)  & 0xff;

        ip = x0 | (x1 <<  8) | (x2 << 16) | (x3 << 24);
    }

    /* always null terminate */
    p  = write_oct(p, ip & 0xff); ip >>= 8; *(p++) = '.';
    p  = write_oct(p, ip & 0xff); ip >>= 8; *(p++) = '.';
    p  = write_oct(p, ip & 0xff); ip >>= 8; *(p++) = '.';
    p  = write_oct(p, ip & 0xff);
    *p = 0;

    strout(info, strbuf, p - strbuf);
}


/*
 * Output a value as a decimal number
 *
 * INPUTS: info = ptr to format data structure
 *         arg_pt = ptr to next argument in list
 */
static void
decout(sfmt_info * info, int do_unsigned, va_list * arg_pt)
{
    char  dec_buff[4 * sizeof(unsigned long)],
        * pt,
        * tmp,
          neg;
    unsigned long uval,       /* unsigned rep. of the input value */
                  count;
    long    len,
            val;    /* signed rep. of the input value */

    count = 0;
    pt    = &dec_buff[19];
    *pt-- = 0;

    if (do_unsigned)
    {
        /* are we doing `short' */
        if (info->mod == 'h')
            uval = (unsigned short) va_arg((*arg_pt), unsigned long);
        else
            uval = (unsigned long) va_arg((*arg_pt), unsigned long);
        
        neg = 0;
    }
    else
    {
        /* are we doing `short' */
        if (info->mod == 'h')
            val = (short) va_arg((*arg_pt), long);
        else
            val = (long) va_arg((*arg_pt), long);

        /* Negative num check. */
        if (val < 0)
        {
            neg = 1;
            val = -val;
        }
        else
            neg = 0;
        
        /* This is guaranteed to be positive! */
        uval = val;
    }


    /*
     * While val is greater than zero, output characters
     * to the string from right to left.
     */
    tmp   = write_u(pt, uval);
    count = pt - tmp;
    pt    = tmp;
#if 0
    do
    {
         *(pt--) = uval % 10 + '0';
         uval /= 10;

        count++;
    } while ( uval );
#endif


    /* Left fill remaining with zeroes. */
    while (count < info->precision)
    {
        * (pt--) = '0';
        count++;
    }

    /* Next check to see if the 0 flag was used in the format
     * and if the current count is less than the specified
     * field width.  If 1 then fill in the leading zeros. 
     * Temporarily leave room in the field for an optional
     * plus or minus sign. */
    pt++;
    if (info->zero)
    {
        len = 1 + strlen(pt);
        while (info->width > len)
        {
            * (--pt) = '0';
            len++;
        }
        len--;
    }

    /* Add leading plus or minus sign, and/or leading zero if
     * needed. */
    if (neg)
        * (--pt) = '-';
    else if (info->plus)
        * (--pt) = '+';
    else if (info->space)
        * (--pt) = ' ';
    else if ( (info->zero) && (info->width > len))
        * (--pt) = '0';
    info->precision = DEFAULT;
    strout(info, pt, strlen(pt));
}


/*
 * Output a value as a hexadecimal number
 * INPUTS: info = ptr to format data structure
 *         arg_pt = ptr to next argument in list
 */
static void
hexout(sfmt_info *info, va_list *arg_pt)
{
    static const char __hexstr[] = "0123456789abcdef";
    static const char __HEXstr[] = "0123456789ABCDEF";
    char  hexbuff[4 * sizeof(unsigned long)],
          * pt;
    const char * d = info->type == 'X' ? __HEXstr : __hexstr;
    unsigned long val, count;
    long len;

    count = 0;

    if (info->mod == 'h')
        val = (unsigned short) va_arg((*arg_pt), unsigned long);
    else
        val = (unsigned long) va_arg((*arg_pt), unsigned long);

    pt = &hexbuff[15];
    *pt-- = 0;

    do
    {
        * (pt--) = d[val & 0xf];
        count++;
    } while (val >>= 4);

    while (count < info->precision)
    {
        * (pt--) = '0';
        count++;
    }

    /* Next check to see if the 0 flag was used in the format
     * and if the current count is less than the specified
     * field width.  If 1 then fill in the leading zeros. 
     * Temporarily leave room in the field for an optional
     * plus or minus sign. */
    pt++;
    if (info->zero)
    {
        len = strlen(pt);
        if (info->alter)
        {
            len += 2;
            while (info->width > len)
            {
                * (--pt) = '0';
                len++;
            }
            len -= 2;
        }
        else
        {
            while (info->width > len)
            {
                * (--pt) = '0';
                len += 1;
            }
        }
    }

    /*
     * If the "alternate form" flag is set, add 0x or 0X to the
     * front of string
     */
    if (info->alter || info->type == 'p')
    {
        if (info->type == 'x' || info->type == 'p')
            * (--pt) = 'x';
        else
            * (--pt) = 'X';
        * (--pt) = '0';
    }
    info->precision = DEFAULT;
    strout(info, pt, strlen(pt));
}


/*
 * percent: Interpret a conversion specification
 *    INPUTS: format = ptr to current position in format string
 *            arg_pt = ptr to next argument
 *
 *    RETURNS: Ptr to next char in format string past this
 *             specifier.
 *    OUTPUTS: *info = filled in with format specification
 */
static const char *
percent(const char *format, sfmt_info *info, va_list *arg_pt)
{
    char p_flag = 0;

    /*  Initialize the f_flag structure with the default values. */
    info->width     = DEFAULT;
    info->precision = DEFAULT_P;
    info->right     = 1;
    info->plus      = 0;
    info->alter     = 0;
    info->zero      = 0;
    info->space     = 0;
    info->type      = 0;
    info->mod       = 0;

    /*
     * Process the flags that may exists between the '%' and the start of
     * the width specification.  The following flags may exist:
     *
     *      '-'    to set left justified
     *      '+'    result will always have sign (+ or -)
     *      space  for leading space
     *      '#'    for alternate form
     *      '0'    use leading zeros instead of spaces.
     *
     * The sequence of these flags is important.
     */

    if (*format == '-')
    {
        info->right = 0;
        format++;
    }

    if (*format == '+')
    {
        info->plus = 1;
        format++;
    }

    if (*format == ' ')
    {
        if (info->plus)
            info->space = 1;
        format++;
    }

    if (*format == '#')
    {
        info->alter = 1;
        format++;
    }

    if (*format == '0')
    {
        if (info->right)
            info->zero = 1;
        format++;
    }

    /*
     * Process the width specification of the format. The width
     * can be specified by the next value in the arg_pt list.
     * If the next character is '*' then get the width from
     * arg_pt with a call to the va_arg () macro.
     *
     * The width can also be specified as a value in the format.
     * In this case the width is specified by a decimal number.
     */
    if (*format == '*')
    {
        info->width = va_arg((*arg_pt), int);
        if (info->width < 0)
        {
            info->right = 0;
            info->width = -info->width;
        }
        format++;
    }
    else
    {
        if (isdigit(*format))
            for (info->width = 0; isdigit(*format); )
                info->width = info->width * 10 + *format++ - '0';
    }

    /* Process the precision field.  The same options that are allowed for
     * the width field are permitted here. */
    if (*format == '.')
    {
        p_flag = 1;
        format++;
        if (*format == '*')
        {
            info->precision = va_arg((*arg_pt), int);
            format++;
        }
        else
        {
            info->precision = 0;
            while (isdigit(*format))
                info->precision = info->precision * 10 + *format++ - '0';
        }
    }

    /*  Check for format modifier characters. {h, l, L} */
    if ( (*format == 'h') || (*format == 'l') || (*format == 'L') )
        info->mod = *format++;

    /*
     * Based on the type of conversion it may be necessary to
     * readjust the precision flag to a default value of 0.
     */
    if ( (p_flag) && (*format != 'c') && (*format != '%'))
        info->precision = 0;

    /* The final step is to read in the conversion type. */
    info->type = *format;
    return format;
}




/*
 * Actual routine that formats output and sends it to
 * the console. This is to implement one common routine for dbg.c
 * and _XPRINT(). It uses a well formed va_list data type sent by the
 * caller.
 * 
 *   INPUTS: format = ptr to format string
 *           Ptr to well-formed varargs data structure
 *           [See _XPRINT() or dbg.c for more details]
 *
 *   NOTE: Supported format strings are explained in the header
 *         at the top of this module.
 */
static void
_print_internal(sfmt_info * info, const char * format, va_list *arg_pt)
{
    unsigned long ch;
    int c;

    /*
     * Process characters in 'format ' string until null
     * terminator is reached.  If the character is not a
     * '%' then simply print it. Otherwise it will require
     * further processing.
     */
    for (; (c = *format); format++)
    {
        if ( c != '%' )
        {
            info->outchar(&info->arg, c);
            continue;
        }

        format = percent(format+1, info, arg_pt);

        /*
         * Continue to process based on the format type:
         *  d,i   -   signed decimal
         *  x,X,p -   unsigned hexadecimal
         *  c     -   print least significant character of int
         *  s     -   argument taken to be (char *) pointer to string
         *  %     -   A % is printed. No argument is converted.
         */

        switch (ch = info->type)
        {
            case 'c':
                charout(info, arg_pt);
                break;
            case 'H':
                ipout(info, 0, arg_pt);
                break;
            case 'N':
                ipout(info, 1, arg_pt);
                break;
            case 'd':
                decout(info, 0, arg_pt);
                break;
            case 'x': case 'X': case 'p':
                hexout(info, arg_pt);
                break;
            case 's':
                stringout(info, arg_pt);
                break;
            case 'u':
                decout(info, 1, arg_pt);
                break;
            case '%':
                info->outchar(&info->arg, '%');
                break;
            default:
                info->outchar(&info->arg, ch);
                break;
        }
    }
}


/* Gateway to the internal routines. The internal routines
 * expect to see an arg of va_list. */
void
v_XPRINT(char * format, va_list * arg_pt)
{
    sfmt_info info;

    if (!format)
        format = (char *)" (null)";
    
    info.arg     = 0;
    info.outchar = putb;
    
    _print_internal(&info, format, arg_pt);
    return ;
}



/*
 * Replacement for sprintf() which avoids things like floats and
 * other unused formats.
 */
int
_XSPRINTF(char * str, const char * format, ...)
{
    va_list ap;
    sfmt_info info;

    assert(format);
    
    info.arg     = str;
    info.outchar = putstr;
    
    va_start(ap, format);
    _print_internal(&info, format, &ap);
    va_end(ap);

    /* Null terminate what's left. */
    *((char *)info.arg) = 0;
    return 0;
}


/*
 * Replcement for printf(). See note at the beginning of this
 * file for a list of supported formats.
 */
void
_XPRINTF(const char * format, ...)
{
    va_list ap;
    sfmt_info info;

    assert(format);
    
    info.arg     = 0;
    info.outchar = putb;
    
    va_start(ap, format);

    _print_internal(&info, format, &ap);
    
    va_end(ap);
}



#ifdef UNIT_TEST
#include <stdio.h>
void
_sys_charout(int ch)
{
    putchar(ch);
}


int
main()
{
    char * ptr = "hello world";
    char buf[1024];
    char buf2[1024];
    uint32_t n = 0xc0a80101,
             h = 0x0101a8c0;

    printf("|%d|\n", 1048576);
    _XPRINTF("|%d|\n", 1048576);

    printf("|%2.2d %2.2d|\n", 1, 20);
    _XPRINTF("|%2.2d %2.2d|\n", 1, 20);

    printf("|%s|\n", ptr);
    _XPRINTF("|%s|\n", ptr);

    printf("|0x%8.8x|\n", 0xdeadbeef);
    _XPRINTF("|0x%8.8x|\n", 0xdeadbeef);

    printf("|0x%8.8X|\n", 0xdeadbeef);
    _XPRINTF("|0x%8.8X|\n", 0xdeadbeef);

    printf("|%p|\n", ptr);
    _XPRINTF("|%p|\n", ptr);
    
    printf("\n<SPRINTF TESTS>:\n");
    
    _XSPRINTF(buf, "|%d|\n", 1048576);
    printf("|%d|\n", 1048576);
    puts(buf);

    _XSPRINTF(buf, "|%u|\n", 1048576);
    printf("|%u|\n", 1048576);
    puts(buf);

    _XSPRINTF(buf, "|%x|\n", 0xdeadbeef);
    printf("|%x|\n", 0xdeadbeef);
    puts(buf);

    _XSPRINTF(buf2, "<%s>\n", buf);
    printf("<%s>\n", buf);
    puts(buf2);

    _XSPRINTF(buf2, "<%N>\n", n);
    printf("<%#x>\n", n);
    puts(buf2);

    _XSPRINTF(buf2, "<%H>\n", h);
    printf("<%#x>\n", h);
    puts(buf2);

    return 0;
}

#endif /* UNIT_TEST */



/* EOF */
