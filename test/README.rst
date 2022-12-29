=====================================================
Test Harness for Portable Library of Useful C/++ code
=====================================================

This directory contains test harness to validate many parts of the
code in the ``src/`` directory. I am documenting the most important
code in this directory.

Building & Running the test suite
=================================
On most POSIX systems (including Darwin)::

    gmake -j5

The object files and exe will be in a OS and build specific directory; the
directory name will have the following convention::

    ./obj-$UNAME-$ARCH-$TYPE/

Where:

    - `$UNAME`: is the lowercase OS name
    - `$ARCH`:  is the common arch name (amd64, aarch64, etc.)
    - `$TYPE`:  is the release type `dbg` or `rel`; where the latter
      have optimizations enabled.

If you wish to build for benchmarking and full optimization, try::

    gmake OPTIMIZE=1 -j5

Guide to Tests
==============
t_mpmcq.c
    Test harness for Multi-producer, Multi-consumer Queue. Uses
    timestamps to record when items are pushed and pulled. It dumps
    sorted (ordered) timestamps and "delta" to stdout. Use the
    output with ``latplot.py`` to see variance in latency.
    Optional arguments: NPRODUCERS NCONSUMERS.

t_spscq.c
    Test harness for Single-producer, Single-consumer queue. It
    verifies consistency of queue operations. It prints a summary of
    performance results upon test completion.

t_hashbench.c
    Benchmark various hash functions by reading tokens (keys) from
    stdin. Prints the hashing speed to stdout (MB/s and cyc/byte).

t_hashspeed.c
    Benchmark various hash functions by using synthetic data of
    various sizes. The sizes are supplied on the command line. e.g.,
    ``./Darwin_objs_rel/t_hashspeed 32 56 64`` will print hash
    function performance for keys of size 32 bytes, 56 bytes and 64
    bytes respectively.

t_bloom.c
    Test harness and benchmark for Bloom filters. Reads tokens from
    stdin or the input file provided on command line.

t_mempool.c
    Pooled memory allocator test harness.

t_fast-ht.c
    Test harness and benchmark for super-fast hash table.

t_fast-ht-basic.c
    Simple test harness for the super-fast hash table.

t_arena.c
    Test harness and benchmark for object-lifetime based memory
    allocator.

zbuf_eg.c
    Example program to show usage of the zlib.h buffered I/O interface (

Multi-threaded Disk Wipe
========================
I wrote an example program to use the user-space arc4random
implementation -- except it turned into a standlone program to wipe
disks using parallel I/O (multiple threads). This is collected in 3
files:

dd-wipe-opt.c
    Auto-generated command-line processing file from the input
    ``dd-wipe-opt.in``.

disksize.c
    Semi portable implementation of grokking disk-size for various
    operating systems. THis has code for Darwin and Linux.

mt-dd-wipe.c
    Main program to drive the disk-wipe logic. Usage:
    ``mt-dd-wipe FILE|DISKNAME``
    Try with ``mt-dd-wipe --help`` on Darwin or Linux.

    **Use this on a live-system at your own risk!**


Tools
=====
``confparse.py``
    Parse a python-ish config file (white-space indented).

``createlog.py``
    Create multiple "log files" for testing file rotation logic.

``genhash.py``
    Scan one or more paths provided on the command line and find all
    "#define" tokens; write them to stdout. Good example of using
    python's multi-processing module to scan files in parallel.

``latplot.py``
    Plot the GET/PUT latencies of the MPMC Queue test harness and show
    a nice graph. Requires 'matplotlib' and 'numpy'.
