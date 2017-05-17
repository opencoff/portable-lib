/* vim: expandtab:sw=4:ts=4:tw=72:
 *
 * Test harness for command line parser.
 */


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include <string>

#include "utils/cmdline.h"

using namespace std;
using namespace putils;


static const char * program_name = "(null)";

// List of options we will recognize
static const command_line_parser::option_spec Opts[]=
{
      {"foreground", "foreground", "F", false, 0,  "no",   "Stay in the foreground"}
    , {"port",       "port",       "p", true, "PORT", "6500", "Listen on port PORT"}
    , {"timeout",    "timeout",    "t", true, "N_SEC", "600",  "Timeout connection after N_SEC seconds"}
    , {"debug",      "debug",      "d", true, "D", "0",    "Set debug level to D"}
};

static const int N_options = sizeof Opts / sizeof Opts[0];


static void
error(int doexit, int errnum, const char * fmt, ...)
{
    fflush(stdout);
    fflush(stderr);

    va_list ap;

    fprintf(stdout, "%s: ", program_name);
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);

    if (errnum)
        fprintf(stdout, ": %s", strerror (errnum));

    fputc('\n', stdout);
    fflush(stdout);

    if (doexit)
        exit(doexit);
}


int
main(int argc, char *argv[])
{
    program_name = argv[0];

    if (argc < 1)
        error(1, 0, "Error: Too few arguments.");

    unsigned int flags = 0;

    // The arguments to the parser is governed by two environment
    // vars: PARSER_IGNORE_UNKNOWN and PARSER_PERMUTE_ARGS

    if (getenv("_IGNORE_UNKNOWN"))
        flags |= command_line_parser::IGNORE_UNKNOWN;
    if (getenv("POSIXLY_CORRECT"))
        flags |= command_line_parser::NO_PERMUTE;

    command_line_parser p(flags);

    p.add(Opts, N_options);
    try
    {
        p.parse(argc, argv);
    }
    catch (command_line_exception& ex)
    {
        error(1, 0, "Exception: %s", ex.what());
    }


    command_line_parser::value v = p["help"];

    if (v)
    {
        printf("%s: A simple program to test command line parsing\n"
               "Usage: %s [options]\n\n"
               "Optional arguments are:\n"
               "%s\n",
               program_name,
               program_name,
               p.help_string().c_str());
        exit(0);
    }

    if ( p["version"])
    {
        printf("%s: $Revision: 1.2 $\n", program_name);
        exit(0);
    }

    for(int i=0; i < N_options; ++i)
    {
        const command_line_parser::option_spec* k = &Opts[i];

        try 
        {
            v = p[k->name];
        }
        catch (command_line_exception& ex)
        {
            error(1, 0, "** Unexpected exception: lookup '%s' => %s",
                        k->name, ex.what());
        }

        if (v)
        {
            printf("OPT-%s: %s\n", k->name, k->req_arg ? v.c_str() : "yes");
        }
    }

    const command_line_parser::argv_array& a = p.args();
    for(size_t i=0; i < a.size(); ++i)
        printf("ARG-%zd: %s\n", i, a[i].c_str());

    return 0;
}



