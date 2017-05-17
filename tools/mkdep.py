#! /usr/bin/env python

#
# Script to parse $(CC) -M output and write a corrected
# dependency file - one that includes the .d file and the .o file as
# targets of the .c//cpp files.
#
# Sudhi Herle <sudhi@herle.net>
#
# This file is in the public domain. Enjoy.
#
# Usage: $0 depfile-name COMMAND-to-run-CPP_M
#
# This script runs well with gcc, armcc, tcc.
#

import os, os.path, sys
from os.path import basename
import subprocess

Z = basename(sys.argv[0])

def error(doex, fmt, *args):
    global Z

    sfmt = "%s: %s" % (Z, fmt)
    if args:
        print >>sys.stderr, sfmt % args
    else:
        print >>sys.stderr, sfmt

    if doex:
        sys.exit(doex)

if len(sys.argv) < 2:
    error(1, "Usage: %s dependency-file-name 'CPP -M command invocation'", Z)


dep = sys.argv[1]

try:
    p   = subprocess.Popen(sys.argv[2:], stdout=subprocess.PIPE, universal_newlines=True)
except Exception, ex:
    error(1, "Unable to execute '%s': %s", sys.argv[2], str(ex))

deps = {}
cont = ''
obj  = ''
for line in p.stdout:
    line = line.strip()
    if line.startswith('#'):
        continue

    if line.endswith('\\'):
        line = line.rstrip('\\')
        cont += line
        cont += ' '
        continue
    else:
        cont += line


    # canonical filepath for Win32
    cont.replace('\\', '/')
    v = cont.split()
    obj = v[0]
    if obj.endswith(':'):
        obj = obj.rstrip(':')
        v = v[1:]

    for e in v:
        deps.setdefault(e, True)
     
    cont = ''

r = p.wait()
if r != 0:
    error(r, "%s failed to run: %d", sys.argv[2], r)

try:
    fd = open(dep, 'w')
    str = "%s %s: " % (dep, obj)
    str += " \\\n\t".join(deps.keys())
    fd.write(str + "\n")
    fd.close()
except Exception, ex:
    error(1, "Unable to write %s: %s", dep, str(ex))


sys.exit(0)
