#! /usr/bin/env python

# Parse a python-like config file where white-space indents.
#
# (c) 2015 Sudhi Herle <sw-at-herle.net> 
#
# Licensing Terms: GPLv2 
#
# If you need a commercial license for this work, please contact
# the author.
#
# This software does not come with any express or implied
# warranty; it is provided "as is". No claim  is made to its
# suitability for any purpose.

import os, sys


def countws(s):
    """Count leading white spaces"""

    n  = 0
    TS = 4

    for c in s:
        if c == ' ':
            n += 1
        elif c == '\t':
            n  = (n // TS + 1) * TS
        else:
            break


    return n, s[n:]


def ws2lvl(ws, init_wl):
    """Convert a whitespace count to an indent level.
    Indentations are measured in units of "init_wl"

    Return the level and if auto-grokked, also the grokked stepsize
    of each indentation level.
    """

    ws_check = [8, 4, 2]

    if ws == 0:
        return 0, init_wl

    if init_wl < 0:
        # Have to grok hard
        for n in ws_check:
            if 0 == ws % n:
                #print "## ws %d, divisor %d => Lvl %d" % (ws, n, ws/n)
                return ws/n, n

        raise Exception("Cannot grok indentation of %d chars" % ws)
    else:
        if 0 != ws % init_wl:
            raise Exception("Indentation level not a multiple of %d" % init_wl)

        return ws/init_wl, init_wl


class bundle(object):
    def __init__(self, **kwargs):
        self.__dict__.update(kwargs)


def fatal(s, *args):
    ps = s % args
    print >>sys.stderr, ps
    sys.exit(1)

class node:

    def __init__(self, par, col, nm):
        self.parent = par
        self.children = []
        self.kv = []
        self.col = col
        self.master = False

        if nm.endswith(':'):
            self.name = nm[:-1]
            self.master = True
            if par:
                pn = par.name
                par.children.append(self)
            else:
                pn = "NONE"

            print "  %s += NEW Master %s at col %d" % (pn, self.name, col)
        else:
            sep = '=' if nm.find('=') > 0 else ' '
            w = nm.split(sep)
            k = w[0]
            v = True if len(w) < 2 else w[1]
            self.key = k
            self.val = v
            if par:
                pn = par.name
                par.kv.append(self)
            else:
                pn = "NONE"

            print "  %s += NEW KV <%s = %s> at col %d" % (pn, k, v, col)


    def __str__(self):
        if self.master:
            s = "Master <%s>" % self.name
        else:
            s = "%s=%s" % (self.key, self.val)

        s += " col %d, parent %s" % (self.col, self.parent.name)
        return s



def parse(fd):
    st     = []
    root   = node(None, -1, "<ROOT>:")
    top    = root
    prev   = root
    prevcol = 0

    n = 0
    E = False
    for line in fd:
        n += 1

        line = line.rstrip()

        c, line = countws(line)

        if line.startswith('#'):
            #print "%d: COMMENT" % n
            continue

        if len(line) == 0:
            #print "%d: EMPTY" % n
            continue

        if c > prevcol:
            if not prev.master:
                fatal("%d: indented node <%12.12s...> doesn't have a parent", n, line)

            print "%d: INDENT push %s col %d; new top %s" % (n, top.name, top.col, prev.name)

            st.append(top)
            top = prev

        elif c < prevcol:
            t  = None
            ok = False
            while len(st) > 0:
                t  = st[-1]
                st = st[:-1]
                print "# top <%s> col %d ?" % (t.name, t.col)
                if t.col < c:
                    ok = True
                    break

                print "%d: DEDENT %s col %d" % (n, t.name, t.col)

            if not ok:
                fatal("%d: Indent stack corrupted near %12.12s ..", n, line)

            top = t
        # fallthru

        nn      = node(top, c, line)
        prev    = nn
        prevcol = c


    return root



def dump(n, l):
    """Dump a node"""

    s = ' ' * (2 * l)
    print "%s <%s>" % (s, n.name)
    for z in n.kv:
        print "%s  %s %s" % (s, z.key, z.val)

    for z in n.children:
        dump(z, l+1)


fd = sys.stdin
if len(sys.argv) > 1:
    fd = open(sys.argv[1])

r = parse(fd)
dump(r, 0)
