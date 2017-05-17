/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * command_line.h - Generic command line parsing utilities
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
 * Creation date: Fri Nov  4 11:55:29 2005
 */

#ifndef __CMDLINE_H_1131126929_33470__
#define __CMDLINE_H_1131126929_33470__ 1

#include <string>
#include <vector>
#include <list>
#include <map>
#include <stdexcept>

#include "utils/strutils.h"

namespace putils {

// command_line parser errors (raised in exception)
enum command_line_error_t
{
      ERR_REQ_ARG_MISSING  = 1
    , ERR_SPURIOUS_ARG
    , ERR_UNKNOWN_OPTION
    , ERR_AMBIG_OPTION
    , ERR_NULL_OPTION_NAME
    , ERR_NULL_KEYWORDS
    , ERR_DUPLICATE_OPTION_NAME
    , ERR_DUPLICATE_LONG_OPTION
    , ERR_DUPLICATE_SHORT_OPTION
    , ERR_UNKNOWN_NAME
};


class command_line_exception : public std::exception
{
public:
    command_line_exception(const std::string& arg, command_line_error_t);
    virtual ~command_line_exception() throw();

    const char * what() const throw();

private:
    static const char * err2str(command_line_error_t);

private:
    std::string m_str;
};



class command_line_parser
{
public:

    // Flags for parser's CTOR
    enum
    {
          NO_PERMUTE     = (1 << 0)     // don't permute arguments
        , NO_HELP        = (1 << 1)     // don't recognize --help
        , NO_VERSION     = (1 << 2)     // don't recognize --version
        , IGNORE_UNKNOWN = (1 << 3)     // ignore unknown options
    };

    // Each command line argument that is a option must be
    // described in an array of this form.
    struct option_spec
    {
        const char*     name;
        const char*     long_opt;
        const char*     short_opt;
        bool            req_arg;
        const char*     meta_var;    // for options that take an argument
        const char*     default_value;
        const char*     help_msg;
    };

    class value
    {
    public:
        value() {}
        value(const std::string& s) : m_str(s), m_set(false)   { }
        value(const char* s) : m_str(s), m_set(false)          { }
        value(const value& v) : m_str(v.m_str), m_set(v.m_set) { }
        virtual ~value()                                       { }

        operator std::string()             { return m_str; }
        operator const std::string() const { return m_str; }
        const char* c_str() const          { return m_str.c_str(); }

        value& operator=(const value& v)
        {
            if (&v != this)
            {
                m_str = v.m_str;
                m_set = v.m_set;
            }
            return *this;
        }


        template <typename T> T as()       { return strtoi<T>(m_str).second; }
        template <bool> bool as()          { return  string_to_bool(m_str);  }

        operator bool() const              { return m_set; }
        bool is_set() const                { return m_set; }

    private:
        friend class command_line_parser;

        // Set this value to 's' and mark the value as having been
        // set manually
        void set(const std::string& s = "")
        {
            m_str = s;
            m_set = true;
        }

        void reset(const std::string& s)
        {
            m_str = s;
            m_set = false;
        }

    private:
        std::string m_str;
        bool        m_set;
    };


    // To hold the list of arguments that are:
    //  - either options we didn't recognize (if IGNORE_UNKNOWN is set)
    //  - permuted args
    //  - arguments at the end of processing
    typedef std::vector<std::string>         argv_array;

public:
    command_line_parser(unsigned int flags = 0);
    ~command_line_parser();

    // Add a set of options that must be recognized.
    // By making this a separate method, we allow incremental
    // construction of the list of options that are of interest.
    void add(const option_spec[], int nopts);


    // Two ways to parse command line arguments
    void parse(int argc, const char * const * argv);
    void parse(const argv_array&);

    // Return list of unknown options we couldn't recognize or,
    // permuted arguments.
    const argv_array& args() const
    {
        return m_args;
    }


    // Return option value identified by 'name'
    // Throws exception if name is not found.
    value operator[](const std::string& name) const ;

    // Generate help string and return it
    const std::string help_string() { return m_help_string; }

    // Utility functions to convert a argc, argv into a vector
    // and vice-versa
    static argv_array make_argv(int argc, const char*const*argv);
    static std::pair<int, const char**> make_argv(const argv_array&);


private:

    // forward decl
    struct option;

    void setup_default_values();

    // Internal routine to parse
    void parse_internal(const argv_array&);

    option * find_long_option(const std::string&) const;
    option * find_short_option(const std::string&) const;

    std::pair<int, option *> find_partial_match(const std::string&) const;

    void validate_input_arg(option *, const std::string&, const std::string&,
                            bool have_arg);

    void parse_long_option(std::string& str);
    void parse_short_option(std::string& str);

    void append_rest(const argv_array&, size_t off);
    void option_sanity_check(const option&);
    void make_help_string(const option&);

private:

    // Internal representation of a option that we are going to
    // recognize
    struct option
    {
        std::string name;
        std::string long_opt;
        std::string short_opt;
        std::string meta_var;    // for options that take an argument
        std::string default_value;
        std::string help_msg;

        bool        req_arg;    // set if this requires an argument

        value       optval;

        option() { }

        option(const option_spec& that)
        {
            assign(that);
        }

        option& operator=(const option_spec& that)
        {
            assign(that);
            return *this;
        }

        void assign(const option_spec& that)
        {
            if (that.name)
                name.assign(that.name);
            if (that.long_opt)
                long_opt.assign(that.long_opt);
            if (that.short_opt)
                short_opt.assign(that.short_opt);
            if (that.meta_var)
                meta_var.assign(that.meta_var);
            if (that.default_value)
                default_value.assign(that.default_value);
            if (that.help_msg)
                help_msg.assign(that.help_msg);

            req_arg = that.req_arg;
        }
    };

    typedef std::map<std::string, option*>  option_dict;
    typedef std::map<std::string, value*>   value_dict;
    typedef std::list<option*>              option_list;


    // Assoc array keyed by long and short options
    // respectively
    option_dict    m_long_opts;
    option_dict    m_short_opts;

    // assoc array for looking up options that are set and their
    // values.
    value_dict     m_values;


    // List of allocated option*. We have to track them here for the
    // simple reason that a given option can be in two tables and it
    // is difficult to know which one to deallocate.
    option_list    m_alloc_list;

    // unrecognized args (and possibly options)
    argv_array      m_args;

    // Help string
    std::string m_help_string;

    bool  m_no_permute,
          m_ignore_unknown,
          m_consume_arg;

    option  * m_prev;
};

} // end of namespace
#endif /* ! __CMDLINE_H_1131126929_33470__ */

/* EOF */
