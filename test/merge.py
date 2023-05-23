#! /usr/bin/env python3

# merge input from multiple runs of genhash.py into a single file
# Remove duplicates as needed

import os, os.path, sys


def main():
    m = dict()
    for fn in sys.argv[1:]:
        m = scan(m, fn)

    gg = ( "%s %s" % (k, v) for k, v in m.items() )
    print("\n".join(gg))

def scan(m, fn):

    if not os.path.isfile(fn):
        return m

    fd = open(fn, "r")
    for l in fd:
        l = l.strip().split()
        if len(l) != 2: continue

        k, v = l[0], l[1]
        m[k] = v

    fd.close()
    return m
        
main()
