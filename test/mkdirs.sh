#! /bin/sh

# Test prog for t_mkdirhier
os=`uname`
rel='dbg'

objdir="./${os}_objs_${rel}"
exe=$objdir/t_mkdirhier
Z=`basename $0`

$exe /tmp /tmp/foo /tmp/foo/bar/none/abc/def
