#! /usr/bin/env python3

#
# Scan /usr/include or any other paths and return a unique list of
# '#define' names we find
#
# Leverages Multiprocessing to exploit every core :-)
# 
# Sudhi Herle <sudhi-at-herle.net>
# License: Public Domain

import os, sys, os.path
import random, string
import re
from os.path import abspath, realpath, join
import multiprocessing as m

Re = re.compile(r'\s*#\s*define\s+(\w+)\s+.*/')

def scan(fn):
    """Scan a file for #defines."""
    global Re

    if not os.path.isfile(fn):
        return {}

    r = Re
    v = {}
    fd = open(fn, 'r')
    for l in fd:
        l = l.strip()
        m = r.match(l)
        if m is not None:
            z = m.group(1)
            v[z] = fn

    fd.close()
    return v

def scantok(fn):
    """Scan a tokenized header file"""
    fd = open(fn, 'r')
    r  = {}
    for l in fd:
        k, v = l.strip().split()
        r[k] = v

    fd.close()
    return r


class mpstate:
    """Multi-process enabled dir walker."""

    def __init__(self):
        self.w = {}
        self.m = m.Pool(processes=None)

    def walk(self, dn):
        """Walk a path, scan every .h file we find 
        and return a list of identifiers"""

        if os.path.isfile(dn):
            func = scan if dn.endswith('.h') else scantok
            x = self.m.apply_async(func, args=(dn,))
            self.w[dn] = x
            return

        if not os.path.isdir(dn):
            return

        for root, dirs, files in os.walk(dn):
            for f in files:
                if not f.endswith('.h'): continue
                fn = join(root, f)
                x = self.m.apply_async(scan, args=(fn,))
                self.w[fn] = x

    def wait(self):
        """Wait for the async scan to complete and return the results."""
        r = {}
        for k, v in self.w.items():
            z = v.get()
            r.update(z)

        return r


darwin_paths = ['/Applications/Xcode.app/Contents', '/Library/DeveloperDA' ]
paths  = ['/usr/include', '/usr/local/include',
        '/opt/local/include',
        'lib/gcc',
        ]
paths += darwin_paths

# Let command line override
if len(sys.argv[1:]) > 0:
    paths = sys.argv[1:]

ms = mpstate()
for pp in paths:
    ms.walk(pp)

r = ms.wait()
for k, v in r.items():
    print("%s %s" % (k, v))
# EOF
