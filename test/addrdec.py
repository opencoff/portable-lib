#! /usr/bin/env python

import os, sys
import re
import subprocess

# Usage: $0 exe regexp [file]

Addr2line = 'addr2line'

def error(doex, fmt, *args):
    sfmt = "%s: %s" % (sys.argv[0], fmt)
    if args:
        str = sfmt % args
    else:
        str = sfmt

    sys.stderr.write(str + '\n')
    if doex:
        sys.exit(doex)


if len(sys.argv) < 3:
    error(1, "Usage: %s exe regexp [file]", sys.argv[0])

exe = sys.argv[1]
rx  = sys.argv[2]

if len(sys.argv) > 3:
    fname = sys.argv[3]
    fd = open(fname)
else:
    fd = sys.stdin


try:
    rxpat = re.compile(rx)
except:
    error(1, "Can't compile regexp '%s'", rx)


try:
    args = [Addr2line, '-e', exe , '-s']
    proc = subprocess.Popen(args, stdout=subprocess.PIPE,
                            stdin=subprocess.PIPE,
                            universal_newlines=True)
except:
    error(1, "Can't run 'addr2line'")


# Feed the lines matching rxpat to addrline and gather output
lines = []
for line in fd:
    line = line.strip()
    m    = rxpat.match(line)
    if m is not None:
        addr = m.group(1)
        proc.stdin.write(addr + '\n')
        decoded = proc.stdout.readline().strip()
        line  = decoded + m.string[m.end(1):]

    print line

