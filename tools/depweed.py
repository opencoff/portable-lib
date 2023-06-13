#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:ts=4:sw=4:expandtab

# Author: Mark Mitchell

"""Script to parse dependency files which were created by gcc -MM -MF,
and validate them.  Those that don't meet validation are deleted.

Validation means:

* dependency file is not empty (has nonzero size)
* all header files listed in the dependency file must exist.

If both these conditions are not met, then the dependency file is deleted.
"""

usage = """Usage: depweed.py [*.d]"""

import sys
import os
import string


class Fail(Exception):
    pass

#    def __init__(self, value):
#        self.value = value
#    def __str__(self):
#        return repr(self.value)


def emit_info(msg):
    sys.stdout.write(msg)

def emit_err(msg):
    sys.stderr.write(msg)

def debug_info(msg):
    return
    emit_info(msg)


def strip_targetname(makerule):
    """Extract just the dependencies from the makefile rule.  The separator
    character between the target and the dependencies is a colon, but there
    a possible complication we need to deal with: on windows, that might not
    be the first or only colon in the rule, as there might also be a colon
    following a windows drive letter on the target filename, or not on the
    target filename but on a dependency filename."""

    tokens = makerule.split(':', 2)
    if len(tokens) > 2:
        # a windows drive letter in some filename
        if len(tokens[0]) == 1 and tokens[0] in string.ascii_letters \
        and (tokens[1].startswith('/') or tokens[1].startswith('\\')):
            # windows drive letter on target filename
            deps = tokens[2]
        else:
            # no drive letter on target filename, but drive letter on some dependency filename
            deps = ':'.join((tokens[1], tokens[2]))
    else:
        deps = tokens[1]

    return deps.strip()


def validate_dep(dfile):
    """Check the .d file is not empty, and that all mentioned dependency
    files exist."""
    if 0 == os.path.getsize(dfile):
        debug_info("size is 0, deleting: %s" % dfile)
        os.unlink(dfile)
        return

    try:
        fd = open(dfile)
    except:
        raise Fail("Can't open for reading: %s" % dfile)

    deplines = []
    for line in fd:
        line = line.strip()
        if not line:
            continue
        deplines.append(line.rstrip(' \\'))

    makerule = ' '.join(deplines)
    deps = strip_targetname(makerule)
    debug_info("deps|%s" % deps)

    ifiles = deps.split()
    for f in ifiles:
        if not os.path.isfile(f):
            debug_info("%s doesn't exist, deleting %s" % (f, dfile))
            os.unlink(dfile)
            return

    # echo the dfile
    print(dfile)

def main(args=None):
    if args is None:
        args = sys.argv[1:]

    if len(args) < 1:
        emit_err('\n'.join((__doc__, usage)))
        return 1

    for depfile in args:
        if os.path.exists(depfile):
            validate_dep(depfile)

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))

