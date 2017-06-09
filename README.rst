====================================
Portable Library of Useful C/++ code
====================================

This directory contains code for many common use cases:

- Bloom filters
- Hash tables
- Variety of good, fast Hash functions
- Typesafe templates in "C"
- Single-Producer, Single-Consumer lock-free bounded queue
- Multi-Producer, Multi-Consumer lock-free bounded queue
- Blocking, bounded, producer-consumer queue
- Thread pool (job-handlers) with CPU affinity using pthreads and a
  shared queue across the threads.
- Round-robin work distribution across N threads using pthreads;
  each thread has its own queue enabling work to be queued to
  specific threads.
- Fixed-size memory allocator ('mempools')
- Growable, resizable string buffer
- Collection of random number generators (ARC4Random-chacha20,
  XORshift, Mersenne-Twister)

Almost all code is written in Portable C (and some C++).  It is
tested to work on at least Linux 3.x/4.x, Darwin (Sierra, macOS),
OpenBSD 5.9/6.0/6.1.

What is available in this code base?
====================================

- Collection of Bloom filters (Simple, Counting, Scalable). The
  Bloom filters can be serialized to disk and read back in mmap
  mode. The serialized code has a strong checksum (SHA256) to
  maintain the integrity of the data when read back. Performance on
  a late 2013 13" MBP (Core i7, 2.8GHz):

    * Standard Bloom filter: 227 cyc/add, 201 cyc/search
    * Counting Bloom filter: 220 cyc/add, 205 cyc/search
    * Scalable Bloom filter: 224 cyc/add, 206 cyc/search

  All the tests were done with false-positive rate of 0.005.

- Multiple implementation of hash tables:

    * Scalable hash table with policy based memory management and
      locking. It resizes dynamically based on load-factor. It has
      several iterators to safely traverse the hash-table. This uses
      a doubly linked list for collision resolution. Performance on a
      late 2013 MBP (Core i7, 2.8GHz):

        - Insert: 611 cyc/add,    4.5 M ops/sec
        - Find:   422 cyc/search, 6.5 M ops/sec
        - Remove: 591 cyc/del,    4.7 M ops/sec

    * A very fast hash table that uses "linked list of arrays" for
      collision resolution. Each such array has 8 elements. The idea
      is to exploit cache-locality when searching for nodes in the
      same bucket. If the collision chain is more than 8 elements, a
      new array of 8 elements is allocated. Performance on a late
      2013 MBP (Core i7, 2.8GHz):

        - Add-empty:      251.5 cy/add      9.47 M/sec
        - Find-existing:  53.2 cy/find     28.38 M/sec
        - Find-non-exist: 53.3 cy/find     27.39 M/sec
        - Del-existing:   53.6 cy/find     28.54 M/sec
        - Del-non-exist:  30.2 cy/find     29.52 M/sec

    * Open addressed hash table that uses a power-of-2 sized bucket
      list and a smaller power-of-2 sized bucket list for overflow.

- Templates in "C" -- these leverage the CPP to create type-safe
  containers for several common data structures:

    * list.h: Single and Doubly linked list (BSD inspired)
    * vect.h: Dynamically growable type-safe "vector" (array)
    * queue.h: Fast, bounded FIFO that uses separate read/write
      pointers
    * stack.h: Fast, bounded LIFO
    * syncq.h: Type-safe, bounded producer/consumer queue. Uses
      POSIX semaphores and mutexes.
    * spsc_bounded_queue.h: A single-producer, single-consumer,
      lock-free queue. Requires C11 (stdatomic.h). Performance on
      late 2013 13" MBP (Core i7, 2.8GHz): ~55 cyc/producer,
      ~ 40 cyc/consumer.
    * mpmc_bounded_queue.h: Templatized version of Dmitry Vyukov's
      excellent lock-free algorithm for bounded multiple-producer,
      multiple-consumer queue. Requires C11 (stdatomic.h).
      Performance on late 2013 13" MBP (Core i7, 2.8GHz) with 4
      Producers and 4 Consumers: 236 cyc/producer, 727 cyc/consumer.

- Portable, inline little-endian/big-endian encode and decode functions
  for fixed-width ordinal types (u16, u32, u64).

- Arbitrary sized bitset (uses largest available wordsize on the
  platform).

- A collection of hash functions for use in hash-tables and other
  places:

    * FNV
    * Jenkins
    * Murmur3
    * Siphash24
    * Metrohash
    * xxHash
    * Superfast hash
    * Hsieh hash
    * Cityhash

  These are benchmarked in the test code *test/t_hashbench.c*.

  If you are going to pick a hash function for use in a hash-table,
  pick one that uses a seed as initializer. This ensures that your
  hash table doesn't suffer DoS attacks.

- A portable, thread-safe, user-space implementation of OpenBSD's
  arc4random(3). This uses per-thread random state to ensure that
  there are no locks when reading random data.

- Implementation of Xorshift+ PRNG: XS64-Star, XS128+, XS1024-Star

- Wrappers for process and thread affinity -- provides
  implementations for Linux, OpenBSD and Darwin.

- gstring.h: Growable C strings library

- zbuf.h: Buffered I/O interface to zlib.h; this enables callers to
  safely call compress/uncompress using user output functions.

- C++ Code:

    * strmatch.h: Templatized implementations of Rabin-Karp,
      Knuth-Morris-Pratt, Boyer-Moore string match algorithms.

    * mmap.h: Memory mapped file reader and writer; implementations
      for POSIX and Win32 platforms exist.


- Specialized memory management:

    * arena.h: Object lifetime based memory allocator. Allocate
      frequently in different sizes, free the entire allocator once.

    * mempool.h: Very fast, fixed size memory allocator; Performance
      on a late 2013 MBP (Core i7, 2.8GHz) is:

        - 55 cyc/alloc, 18M allocs/sec
        - 55 cyc/free,  18M frees/sec

- OSX Darwin specific code:

    * POSIX un-named semaphores
    * C11 stdatomic.h
    * Replacement for <time.h> to include POSIX clock_gettime().
      This is implemented using Mach APIs.


- Portable routines to read password (POSIX and Win32)

- POSIX compatible wrappers for Win32: mmap(2), pthreads(7),
  opendir(3), inet_pton(3) and inet_ntop(3), sys/time.h

- Portable implementation of getopt_long(3).

How is porability achieved?
---------------------------
The code above tries to be portable without use of ``#ifdef`` or
other pre-processor constructs. In cases where a particular platform
does not provide a required symbol or function, a compatibility
header is provided in ``inc/$PLATFORM/``. e.g., Darwin doesn't have
a working POSIX un-named semaphore implementation (``sem_init(3)``);
the file ``inc/Darwin/semaphore.h`` provides a working
implementation of the API. Thus, any program using un-named
semaphores can function without any wrappers or ugly ``ifdef``.

While the compatibility functions and symbols are provided via the
mechanism above, the next question is - "how does one tailor the
build environment to accommodate these peculiarities?". This is
where we leverage features of ``make`` to have a conditional build
environment.

GNUmakefile Tricks and Tips
~~~~~~~~~~~~~~~~~~~~~~~~~~~
This library comes with a set of ``GNUmake`` fragments and an
example top-level ``GNUmakefile`` to make building programs easy.

These makefiles are written to be cross-platform and incorporates
many idioms to make building for multiple platforms possible
**without** needing the bloated ``configure`` infrastructure.

For each platform that is supported, ``portablelib.mk`` defines a
set of macros for that platform like so::

    Darwin_incdirs += /opt/local/include /usr/local/include
    Darwin_ldlibs  += /opt/local/lib/libsodium.a
    Darwin_objs    += darwin_cpu.o darwin_sem.o darwin_clock.o

    Linux_defs   += -D_GNU_SOURCE=1
    Linux_ldlibs += -lpthread
    Linux_objs   += linux_cpu.o arc4random.o

    OpenBSD_ldlibs += -L/usr/local/lib -lsodium -lpthread
    OpenBSD_objs   += openbsd_cpu.o


Then, these flags are used to set ``CFLAGS`` and ``objs`` via
"double variable expansion"  like so::

    platform := $(shell uname -s)

    INCDIRS = $($(platform)_incdirs) $(TOPDIR)/inc/$(platform) $(TOPDIR)/inc 

    INCS = $(addprefix -I, $(INCDIRS))
    DEFS = -D__$(platform)__=1 $($(platform)_defs)

    CFLAGS = -g -O2 $(INCS) $(DEFS)
    LDFLAGS = $($(platform)_ldlibs)


In similar fashion, the list of object files to be built is expanded
to include platform specific object files.
This Makefile feature allows us to separate platform specific
peculiarities without the mess of ``autoconf`` and ``automake``.

What is in the *tools/* subdirectory?
=====================================
The *tools* subdirectory has several utility scripts that are useful
for the productive programmer.

mkgetopt.py
-----------
This script generates command line parsing routines from a human readable
specification file. For more details, see *tools/mkgetopt-manual.rst*.
A fully usable example specification is in *tools/example.in*.

depweed.py
----------
Parse ``gcc -MM -MD`` output and validate each of the dependents. If
any dependent file doesn't exist, then the owning ``.d`` file is
deleted. This script is most-useful in a GNUmakefile: instead of
``include $(depfiles)``, one can now do::

    include $(shell depweed.py $(depfiles))

This makes sure that invalid dependencies never make it into the
Makefile.

.. vim: ft=rst:sw=4:ts=4:expandtab:tw=78:
