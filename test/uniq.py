#! /usr/bin/env python3

# read a file or stdin of "key val" lines and remove duplicate keys
# sw-at-herle.net Aug 2023
#
import os, sys, random

def main():
    m = dict()

    if len(sys.argv[1:]) == 0:
        m = scan(sys.stdin, m)
    else:
        for fn in sys.argv[1:]:
            try:
                fd = open(fn, "r")
            except Exception as ex:
                warn("can't open %s: %s", fn, str(ex))
                continue

            m = scan(fd, m)
            fd.close()

    # now print the entries
    v = [ "%s %s" % (k,v) for k, v in m.items() ]
    random.shuffle(v)
    print("\n".join(v))


def scan(fd, m):
    for l in fd:
        l = l.strip()
        i = l.find(" ")
        if i < 0: continue

        k = l[:i]
        v = l[i+1:]
        if k in m: continue

        m[k] = v

    return m

main()
