#! /usr/bin/env python

# Simple script to analyze the output of t_hashfunc for collisions
# etc. Later on we can add other tests to verify the ability of hash
# functions to "mix" the keys.
#
# (c) 2011 Sudhi Herle <sw at herle.net>
# License: GPLv2
#
import os, sys, os.path
from os.path import basename
from optparse import Option, OptionParser

Z     = basename(sys.argv[0])
zhelp = """Usage: %(prog)s [options] [input file]

%(prog)s analyzes the output of t_hashfunc for hash collisions. It
optionally prints out possible allocation of these hashes into a
hash table of user defined size.
""" % { 'prog': Z }

class hashmap:
    """counting number of times hashvalue has been found"""

    def __init__(self, name):
        self.name = name
        self.d = dict()


    def update(self, key):
        k = int(key, 16)
        self.d.setdefault(k, 0)
        self.d[k] += 1

    def collisions(self):
        """Return number of collisions"""

        n = 0
        for v in self.d.values():
            if v > 1:
                n += 1

        return n


class hashtable(object):
    """Simulation of a simple hash table"""

    def __init__(self, name, N, R):
        self.name = name
        self.N = N
        self.R = R
        self.t = [list() for i in range(N)]

        self.fill  = 0
        self.nodes = 0
        self.maxchainlen = 0
        self.splits = 0



    def update(self, val):
        """Add element v to hash table"""

        v = int(val, 16)
        b = v % self.N
        r = self.t[b]
        r.append(v)

        self.nodes += 1

        #print val, b, self.name, ','.join(["%8.8x" % x for x in r])
    
        k = len(r)
        if k > self.maxchainlen:
            self.maxchainlen = k

        if k == 1:
            self.fill += 1
            rate = 100* float(self.fill)/float(self.N) 

            if rate > self.R:
                self.rehash()


    def rehash(self):

        n = self.N * 2
        t = [ list() for i in range(n) ]
        f = 0
        m = 0

        self.splits += 1

        for b in self.t:
            for e in b:
                newb = e % n
                buk  = t[newb]
                buk.append(e)
                r    = len(buk)
                if r > m:
                    m = r
                if r == 1:
                    f += 1

        self.fill = f
        self.t    = t
        self.N    = n
        self.maxchainlen = m


    def dump(self):

        i = 0
        print("%s: %d buckets, %d nodes" % (self.name, self.N, self.nodes))
        for b in self.t:
            print("  %3d: %s" % (i, ','.join(["%8.8x" % x for x in b])))
            i += 1

    def printstats(self):

        t = {'fillrate': self.fillrate,
             'density': self.density,
             'realdensity': self.realdensity,
             'avgchainlen': self.avgchainlen
             }
        t.update(self.__dict__)
        print("""%(name)s: %(N)d buckets, %(nodes)d nodes
    Fill rate:    %(fillrate)3.2f %%
    Nodes/bucket: exp %(density)3.2f, actual %(realdensity)3.2f
    Chain length: max %(maxchainlen)d, avg %(avgchainlen)d""" % t)

    def chainlen(self, b):
        """Return chainlen for bucket b"""

        assert b < N and b >= 0, "Bucket %d is out of range" % b
        return self.t[b]


    @property
    def fillrate(self):
        """Bucket fill rate"""
        return 100.0 * float(self.fill)/float(self.N)

    @property
    def density(self):
        return float(self.nodes)/float(self.N)

    @property
    def realdensity(self):
        if self.fill > 0:
            return float(self.nodes)/float(self.fill)

        return 0

    @property
    def avgchainlen(self):
        return int(self.realdensity)


def find_collisions(fp, hashtab=False, N=0, R=0):
    header = fp.readline().strip()
    names  = header.split(',')
    hashes = [hashmap(x) for x in names]
    dist   = None

    if hashtab:
        dist = [hashtable(x, N, R) for x in names]

    while True:
        l = fp.readline().strip()
        if not l:
            break

        vals = l.split(',')
        for h, v in zip(hashes, vals):
            h.update(v)

        if dist:
            for h, v in zip(dist, vals):
                h.update(v)


    coll =  [ h.collisions() for h in hashes ]
    have_coll = [ z for z in coll if z > 0 ]
    if len(have_coll) > 0:
        print("Collisions:")
        print("  ", "  ".join(["{:^8}".format(x) for x in names]))
        print("  ", "  ".join(["{:^8d}".format(x) for x in coll]))


    return dist

def main():
    parser = OptionParser(zhelp)
    parser.add_option("-t", "--table-size", dest="N", action="store", 
                      type="int", default=0, metavar='N',
                      help="Print distribution to a hash table of size 'N' [%default]")
    parser.add_option("-f", "--fill-rate", dest="fill", action="store",
                      type="float", default=70, metavar='r',
                      help="Set hash table fill rate to 'r' [%default]")

    (opt, args) = parser.parse_args()
    argc = len(args)
    fp = sys.stdin
    if argc > 0:
        fp = open(args[0], 'r')


    if opt.N > 0:
        d = find_collisions(fp, True, opt.N, opt.fill)
    else:
        d = find_collisions(fp)

    if d:
        for h in d:
            #h.dump()
            h.printstats()

main()
