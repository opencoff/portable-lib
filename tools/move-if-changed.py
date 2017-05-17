#! /usr/bin/env python

#
# A small script to over-write a destination file if:
#   (a) It doesn't exist
#   (b) If the source is newer than the destination
#
# In otherwords, it is a smart version of "cp".
#
# (c) 2005-2010 Sudhi Herle <sudhi@herle.net>
#
# This file is in the public domain. Enjoy!
#
import os, os.path, sys, shutil
from optparse import Option, OptionParser, OptionValueError

Z       = os.path.basename(sys.argv[0])
__doc__ = """%s [options] SRC-FILE DEST-FILE

%s - A more intelligent 'cp' that copies SRC-FILE to DEST-FILE if one of
the two conditions below are met:

   - DEST-FILE doesn't exist
   - SRC-FILE is newer than DEST-FILE
""" % (Z, Z)

def die(fmt, *args):
    """Die with an error message"""

    sfmt = "%s: %s" % (sys.argv[0], fmt)
    if args:
        str = sfmt % args
    else:
        str = sfmt

    print >>sys.stderr, str
    sys.exit(1)

def filestat(fname):
    try:
        st = os.stat(fname)
        return st.st_mtime, st.st_size
    except:
        return -1, 0


def different(a, b):
    """Return true if a & b are different in contents"""
    return True


p = OptionParser(__doc__)
p.add_option("-v", "--verbose", dest="verbose", action="store_true",
             default=False, help="Be verbose about actions [%default]")

opt, args = p.parse_args()

if len(args) < 3:
    die("Usage: %s SRC-FILE DEST-FILE", sys.argv[0])

src  = args[1]
dest = args[2]


s_mtime, s_size = filestat(src)
d_mtime, d_size = filestat(dest)

if s_mtime < 0:
    die("Can't query source file %s", src)

if d_mtime < 0 or s_mtime > d_mtime or different(src, dest):
    if opt.verbose:
        print "cp %s %s" % (src, dest)
    shutil.copy2(src, dest)


# vim: notextmode:expandtab:sw=4:ts=4:tw=72:
