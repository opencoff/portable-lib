#! /usr/bin/env python3

# Usage: $0 exe prof.out
#
# This script is to be used in conjunction with
# "-finstrument-functions" and profile.c (in the same dir).
#
# It will print a call-tree of functions so that one may get a
# "view" of the callers and callees of various APIs.
#
# (c) 2005, 2006, 2007 Sudhi Herle <sudhi@herle.net>
# License: GPLv2
# 

import os, sys


if len(sys.argv) < 3:
    print >>sys.stderr, "Usage: %s EXE prof.out" % sys.argv[0]
    sys.exit(1)


class bundle:
    def __init__(self, **kwargs):
        self.__dict__.update(kwargs)

def pad(lev):
    if lev <= 0:
        return ""
    return '    ' * lev


exe   = sys.argv[1]
prof  = sys.argv[2]
symfd = os.popen("nm %s" % exe, "r")

# Read syms into hash
symhash = {}
for l in symfd:
    v = l.strip().split()
    if len(v) < 3:
        continue

    addr, typ, sym = v
    addr = int(addr, 16)
    symhash[addr] = bundle(addr=addr, type=typ, sym=sym)

symfd.close()
pfd   = open(prof, "r")
level = -1
empty = bundle(addr=0, type='', sym='')
for l in pfd:
    typ, fn, caller = l.strip().split()
    if typ == 'E':
        level += 1

    fn = int(fn, 16)
    caller = int(caller, 16)
    fn_rec = symhash.get(fn)
    if fn_rec is None:
        fsym = "%#x" % fn
    else:
        fsym = fn_rec.sym

    caller_rec = symhash.get(caller)
    if caller_rec is None:
        csym = "%#x" % caller
    else:
        csym = caller_rec.sym

    print "%s %s %s %s" % (pad(level), typ, fsym, csym)
    if typ == 'X':
        level -= 1

pfd.close()
