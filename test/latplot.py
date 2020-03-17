#! /usr/bin/env python

# MPMCQ Latency Plotter
#  - Reads a list of latency numbers from command line files and
#    averages them
#  - Plots a simple x,y chart
#
# Sudhi Herle <sudhi-at-herle.net>
#
# License: Public Domain

import os, sys

import matplotlib.pyplot as plt
import numpy as np

class samples:
    """Gather latency samples and average them"""
    def __init__(self):
        self.lat = []

    def readcsv(self, fd):
        lat = []
        ymax = 0
        ymin = 0xfffffffff
        for l in fd:
            if l.startswith('#'):
                continue

            z = int(l)
            lat.append(z)

        if len(self.lat) == 0:
            self.lat  = lat
        else:
            # Otherwise, calculate average and update
            print("# averaging %d existing with %d new seq" % (len(self.lat), len(lat)))
            self.lat = [ (a + b)/2.0 for a, b in zip(self.lat, lat) ]

    def result(self):
        """Return min, median and max"""
        z = sorted(self.lat)
        n = len(z)
        if 1 == n % 2:
            m = z[n // 2]
            return m, m, m
        
        i  = n // 2

        ml = z[i-1]
        mh = z[i]

        # Median = ml+mh/2

        small = z[0]
        large = z[-1]
        return small, (ml+mh)/2, large

    def __len__(self):
        return len(self.lat)


def median(d):
    """Return median_low, median, median_high"""

    z = sorted(d)
    n = len(z)
    if 1 == n % 2:
        m = z[n // 2]
        return m, m, m
    
    i  = n // 2
    ml = z[i-1]
    mh = z[i]
    return ml, (ml+mh)/2, mh



ss = samples()
for f in sys.argv[1:]:
    fn = sys.argv[1]
    fd = open(fn, 'r')
    ss.readcsv(fd)
    fd.close()

ymin, mm, ymax = ss.result()
print("# %d samples, min %d max %d median %d" % (len(ss), ymin, ymax, mm))

x = range(len(ss))
y = ss.lat
#for z in zip(s, l):
    #print z[0], z[1]

lines = plt.plot(x, y)
plt.title("MPMC Average Latency Chart")
plt.xlim(0, len(ss))
plt.ylim(0, ymax)
plt.ylabel("Latency")
plt.xlabel("Sequence")
plt.grid(True)
plt.show()
