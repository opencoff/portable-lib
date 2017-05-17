#! /usr/bin/env python

import os, sys

# Simple script to create a bunch of log files; useful for testing
# rotatefile() [t_rotatefile]
#
# Sudhi Herle <sudhi@herle.net>
#
def create_file(f):
    fd = open(f, "w")
    print >>fd, f
    fd.close()

n = len(sys.argv)
if n < 3:
    print >>sys.stderr, "Usage: %s FILENAME NSAVED" % sys.argv[0]
    sys.exit(0)

filename = sys.argv[1]
nsaved   = int(sys.argv[2])

while nsaved >= 0:
    f = filename + ".%d" % nsaved
    create_file(f)
    nsaved -= 1

create_file(filename)
