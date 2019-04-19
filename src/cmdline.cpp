/* vim: expandtab:tw=68:ts=4:sw=4:
 *
 * cmdline.cpp - Command line parsing utilities
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
 * Developer Notes
 * ===============
 * Each option that is recognized by the parser has the following
 * attributes:
 *      - name: unique identifying name; used to query whether said
 *              option was set on the command line or not.
 *      - long_opt:  Long option name
 *      - short_opt: Short option name
 *      - req_arg:  Bool flag - set if the option requires an
 *                  argument (or value)
 *      - meta_var: For options requiring arguments, this
 *                  symbolically represents the value; used in
 *                  generation of help string.
 *      - help_msg: Help string
 *      - value:    The struct representing the value of each
 *                  recognized option.
 *
 * This information is represented by 'struct option'.
 *
 * To make lookup faster, we maintain three different assoc. arrays:
 *     - long-opt lookup table: indexed by long opt
 *     - short-opt lookup table: indexed by short opt
 *     - values lookup table: indexed by option name
 */

#include "utils/cmdline.h"


using namespace std;

namespace putils {



// Return true if 'str' is (the start of) a long option
static bool
is_long_opt(const string& str)
{
    return str.length() > 2 && str[0] == '-' && str[1] == '-';
}


// Return true if 'str' denotes end of option processing
static bool
is_opt_end(const string& str)
{
    return str.length() == 2 && str[0] == '-' && str[1] == '-';
}


// Return true if 'str' is (the start of) a short option
static bool
is_short_opt(const string& str)
{
    return str.length() > 1 && str[0] == '-';
}


// Return true of 'lhs' is a prefix of 'rhs'. This is used for
// fuzzy(partial) matches of long options.
static bool
is_prefix_of(const string& lhs, const string& rhs)
{
    // degenerate case
    if (lhs.length() == 0 || rhs.length() == 0)
        return false;

    // "prefix" implies lhs length must be less than rhs length
    if (lhs.length() > rhs.length())
        return false;

    for(string::size_type i = 0; i < lhs.length(); ++i)
        if (lhs[i] != rhs[i])
            return false;

    return true;
}


// Return a string of white spaces such that when concatenated with
// another string of length 'cur_length', it is padded upto
// column 30.
string
pad(string::size_type cur_length)
{
    static const string::size_type max_len = 30;
    string s;

    if (cur_length >= max_len)
        return s;

    string::size_type pad_length = max_len - cur_length;
    while (pad_length-- > 0)
    {
        s += " ";
    }
    return s;
}



// -- Public Methods of putils::command_line_parser --


command_line_parser::command_line_parser(unsigned int flags)
{
    m_no_permute        = !!(flags & NO_PERMUTE);
    m_ignore_unknown    = !!(flags & IGNORE_UNKNOWN);

    if (!(flags & NO_HELP))
    {
        static const option_spec opt =
        {
            "help",     // name
            "help",     // long_opt
            "h",        // short_opt
            false,      // req_arg
            "",         // meta_var
            "no",       // default_value
             "Show help message and quit" // help
        };

        add(&opt, 1);
    }

    if (!(flags & NO_VERSION))
    {
        static const option_spec opt =
        {
            "version",  // name
            "version",  // long_opt
            "V",        // short_opt
            false,      // req_arg
            "",         // meta_var
            "no",       // default_value
            "Show program version and quit" //help
        };

        add(&opt, 1);
    }
}



command_line_parser::~command_line_parser()
{
    option_list::iterator i = m_alloc_list.begin();

    for (; i != m_alloc_list.end(); ++i)
    {
        option * opt = *i;

        delete[] opt;
    }
}


// Add a set of options that must be recognized.
// By making this a separate method, we allow incremental
// construction of the list of options that are of interest.
// This function will also build up the help string along the way.
void
command_line_parser::add(const option_spec opt_spec[], int n_opt)
{
    option * opts = new option[n_opt];

    for(int i=0; i < n_opt; ++i)
    {
        const option_spec& s = opt_spec[i];
        option* d    = &opts[i];

        *d = s;

        option_sanity_check(*d);

        // insert into all the tables

        if (d->long_opt.length() > 0)
            m_long_opts[d->long_opt] = d;

        if (d->short_opt.length() > 0)
            m_short_opts[d->short_opt] = d;

        m_values[d->name] = &d->optval;

        make_help_string(*d);
    }

    setup_default_values();

    // keep track of arrays we've allocated
    m_alloc_list.push_back(opts);
}


// parse an argc, argv
void
command_line_parser::parse(int argc, const char * const * argv)
{
    // Convert C style arguments into vector<>
    argv_array a = make_argv(argc, argv);

    parse_internal(a);
}


// parse a vector of strings
void
command_line_parser::parse(const command_line_parser::argv_array& av)
{
    // We make a copy of the table -- in case 'args' is the same as
    // 'm_args'.
    argv_array a(av);

    parse_internal(a);
}



// public accessor methods
command_line_parser::value
command_line_parser::operator[](const std::string& name) const
{
    value_dict::const_iterator v = m_values.find(name);
    if (v == m_values.end())
        throw command_line_exception(name, ERR_UNKNOWN_NAME);
    return *v->second;
}

// -- Static public methods --


command_line_parser::argv_array
command_line_parser::make_argv(int argc, const char* const* argv)
{
    argv_array a;

    a.reserve(argc);
    for(int i = 0; i < argc; ++i)
        a.push_back(argv[i]);
    return a;
}


pair<int, const char**>
command_line_parser::make_argv(const argv_array& a)
{
    int argc = a.size();
    const char** argv = new const char*[a.size()];

    for(argv_array::size_type i = 0; i < a.size(); ++i)
        argv[i] = a[i].c_str();

    return make_pair(argc, argv);
}

// -- Implementation of other classes --


command_line_exception::command_line_exception(const string& str, command_line_error_t e)
    : exception(),
      m_str(str)
{
    if (m_str.length() > 0)
        m_str += ":";

    m_str += string(err2str(e));
}


command_line_exception::~command_line_exception() throw()
{
}


const char*
command_line_exception::what() const throw()
{
    return m_str.c_str();
}


// map error enum to a human readable string
const char*
command_line_exception::err2str(command_line_error_t e)
{
    // XXX Keep this table in sync with command_line_error_t
    static const char* strs[] =
    {
          "Missing required argument"
        , "Argument supplied where none expected"
        , "Unrecognized option"
        , "Ambiguous option"
        , "Keyword name is empty"
        , "Long and short options are empty"
        , "Duplicate option name"
        , "Duplicate long option"
        , "Duplicate short option"
        , "Unknown option name"
    };
    return strs[e];
}



// -- Private Methods of cmdline::parser --


// Actual routine to parse command line arguments
void
command_line_parser::parse_internal(const command_line_parser::argv_array& av)
{
    // Reset important temporary tables and initialize values to
    // their defaults

    m_args.clear();
    setup_default_values();

    m_consume_arg = false;

    // skip program name (argv[0])
    // and start from index '1'.
    for(size_t i = 1; i < av.size(); ++i)
    {
        string arg(av[i]);

        // This must be the _first_ thing we check
        if (is_opt_end(arg))
        {
            // consume the args[i]; it's a lone "--".
            append_rest(av, i+1);
            return;
        }

        // consume 'arg' if the previous option required an argument
        // to be passed
        if (m_consume_arg)
        {
            m_prev->optval.set(arg);
            m_prev        = 0;
            m_consume_arg = false;

            continue;
        }

        if (is_long_opt(arg))
        {
            arg = arg.substr(2);
            parse_long_option(arg);
        }
        else if (is_short_opt(arg))
        {
            arg = arg.substr(1);
            parse_short_option(arg);
        }
        else
        {
            // Not a short opt or a long opt.
            if (m_no_permute)
            {
                // stop at the first non-option
                append_rest(av, i);
                return;
            }
            else
                m_args.push_back(arg);
        }
    }
}



// Find an exact match for long option 'str'
command_line_parser::option *
command_line_parser::find_long_option(const string& str) const
{
    option_dict::const_iterator p = m_long_opts.find(str);
    return p == m_long_opts.end() ? 0 : p->second;
}


// Find an exact match for short option 'str'
command_line_parser::option *
command_line_parser::find_short_option(const string& str) const
{
    option_dict::const_iterator p = m_short_opts.find(str);
    return p == m_short_opts.end() ? 0 : p->second;
}


// Iterate through all long options and find a partial (prefix)
// matches for 'arg'. The first element of the pair<> is the number
// of matches.
pair<int, command_line_parser::option *>
command_line_parser::find_partial_match(const string& arg) const
{
    int nmatches = 0;
    option * opt = 0;

    option_dict::const_iterator i = m_long_opts.begin();
    for (; i != m_long_opts.end(); ++i)
    {
        const string& optstr = i->first;
        if (is_prefix_of(arg, optstr))
        {
            nmatches++;
            opt = i->second;
        }
    }

    return make_pair(nmatches, opt);
}



// append rest of command line arguments into our array
void
command_line_parser::append_rest(const command_line_parser::argv_array& av, size_t off)
{
    for(size_t i = off; i < av.size(); ++i)
        m_args.push_back(av[i]);
}


// Validate option 'str' and corresponding 'arg'.
void
command_line_parser::validate_input_arg(command_line_parser::option *opt, const string& str,
                         const string& arg, bool have_arg)
{
    if (opt->req_arg)
    {
        if ( have_arg )
        {
            if (arg.length() == 0)
                throw command_line_exception(str, ERR_REQ_ARG_MISSING);

            opt->optval.set(arg);
        }
        else
        {
            // Consume the next cmdline argument (irrespective
            // of whether it starts with "--" or "-")
            m_consume_arg = true;
            m_prev        = opt;
        }
    }
    else if (arg.length() > 0)
        throw command_line_exception(str, ERR_SPURIOUS_ARG);
    else
        opt->optval.set();
}


// (Attempt to) recognize a long option
void
command_line_parser::parse_long_option(string& str)
{
    string arg;
    bool have_arg = false;

    // First split into str=argue
    size_t p = str.find('=');

    if (p != string::npos)
    {
        arg      = str.substr(p+1);
        str      = str.substr(0, p);
        have_arg = true;
    }


    // First try exact match
    option * opt = find_long_option(str);
    if (!opt)
    {
        pair<int, option*> r = find_partial_match(str);
        int& n_matches       = r.first;

        if (n_matches == 0)
        {
            if (m_ignore_unknown)
            {
                // Reconstruct original cmdline option
                string x("--");

                x += str;
                if (have_arg)
                {
                    x += "=";
                    x += arg;
                }
                m_args.push_back(x);
            }
            else
                throw command_line_exception(str, ERR_UNKNOWN_OPTION);
        }
        else if (n_matches > 1)
            throw command_line_exception(str, ERR_AMBIG_OPTION);

        // We are guaranteed that this is the sole matching option
        opt = r.second;
    }

    // found an exact match
    validate_input_arg(opt, str, arg, have_arg);
}


// (Attempt to) recognize a short option
void
command_line_parser::parse_short_option(string& str)
{
    string arg = str.substr(1);

    str.erase(1); // erase everything after 1

    // First try exact match
    option * opt = find_short_option(str);
    if (!opt)
    {
        if (m_ignore_unknown)
        {
            string x("-");
            x += str;
            if (arg.length() > 0)
                x += arg;

            m_args.push_back(x);
        }
        else
            throw command_line_exception(str, ERR_UNKNOWN_OPTION);

        return;
    }

    validate_input_arg(opt, str, arg, arg.length() > 0);
}


// verify that opt.long_opt and opt.short_opt are not present in any
// tables. Additionally, verify that opt.name is not present in
// values_table as well
void
command_line_parser::option_sanity_check(const option& opt)
{
    if (opt.name.length() == 0)
        throw command_line_exception(opt.name, ERR_NULL_OPTION_NAME);

    if (opt.long_opt.length() == 0 && opt.short_opt.length() == 0)
        throw command_line_exception(opt.name, ERR_NULL_KEYWORDS);

    value_dict::const_iterator vi =  m_values.find(opt.name);
    if (vi != m_values.end())
        throw command_line_exception(opt.name, ERR_DUPLICATE_OPTION_NAME);

    option_dict::const_iterator oi = m_long_opts.find(opt.long_opt);
    if (oi != m_long_opts.end())
        throw command_line_exception(opt.long_opt, ERR_DUPLICATE_LONG_OPTION);

    oi = m_short_opts.find(opt.short_opt);
    if (oi != m_short_opts.end())
        throw command_line_exception(opt.short_opt, ERR_DUPLICATE_SHORT_OPTION);
}


// setup default values for every value in the table
// This naive implementation (iterating through two tables
// likely processing some options twice) is sufficient for command
// line processing needs. It's not likely to be used in some
// performance critical inner loop.
void
command_line_parser::setup_default_values()
{
    option_dict::iterator k = m_long_opts.begin();

    for (; k != m_long_opts.end(); ++k)
        k->second->optval.reset(k->second->default_value);

    for (k = m_short_opts.begin(); k != m_short_opts.end(); ++k)
        k->second->optval.reset(k->second->default_value);
}


// Turn a option spec into a help string
void
command_line_parser::make_help_string(const command_line_parser::option& opt)
{
    string s;

    s.reserve(80);

    s += "    ";

    bool have_long_opt  = opt.long_opt.length() > 0;
    bool have_short_opt = opt.short_opt.length() > 0;
    if (have_long_opt)
    {
        s += "--";
        s += opt.long_opt;
        if (opt.req_arg)
        {
            s += "=";
            s += opt.meta_var;
        }
    }

    if (have_short_opt)
    {
        if (have_long_opt)
            s += ", ";

        s += "-";
        s += opt.short_opt;
        if (opt.req_arg)
            s += opt.meta_var;
    }

    s += pad(s.length());
    s += opt.help_msg;
    s += " [";
    s += opt.default_value;
    s += "]";
    s += "\n";

    m_help_string += s;
}




} // end of namespace putils


/* EOF */
