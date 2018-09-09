
# Common fragment for people using this library
# Usage in your GNUmakefile:
#
#  - define PORTABLE to path to this top level directory
#  - include $(PORTABLE)/portablelib.mk

ifeq ($(PORTABLE),)
	junk := $(error ** Please define PORTABLE before including this makefile **)
endif

ifeq ($(platform),)
	platform := $(shell uname -s)
endif


all_win32_objs += gettimeofday.o truncate.o ndir.o \
					ftruncate.o sleep.o basename.o dirname.o glob.o \
					winmmap.o errno.o fsync.o inetutils.o \
					socketpair.o

win32_vpath   += $(PORTABLE)/src/win32
win32_incdirs += $(PORTABLE)/inc/win32
win32_defs    += -DWIN32 -D_WIN32 -DWINVER=0x0501
win32_tests   +=
win32_ldflags +=
win32_ldlibs  += -lwsock32

all_posix_objs = daemon.o

#all_posix_objs += resolve.o
all_posix_objs += c_resolve.o work.o job.o

posix_vpath    += $(PORTABLE)/src/posix
posix_incdirs  +=
posix_defs     +=
posix_tests    +=
posix_ldlibs   +=
posix_ldflags  +=

Linux_ldlibs     += -lsodium
Linux_defs       += -D_GNU_SOURCE=1

Darwin_incdirs   += /opt/local/include
Darwin_ldlibs    += /opt/local/lib/libsodium.a

OpenBSD_incdirs  += /usr/local/include
OpenBSD_ldlibs	 += -L/usr/local/lib -lsodium -lpthread

posix_oses  = Linux Darwin Solaris OpenBSD FreeBSD NetBSD

# Helper for Posix OSes
ifneq ($(findstring $(platform),$(posix_oses)),)
	$(platform)_vpath   += $(posix_vpath)
	$(platform)_incdirs += $(PORTABLE)/inc/$(platform) $(posix_incdirs)
	$(platform)_defs    += $(posix_defs)
	$(platform)_ldflags += $(posix_ldflags)
	$(platform)_ldlibs  += $(posix_ldlibs)
endif


# Default objs for linux 
Linux_defobjs     = $(all_posix_objs) linux_cpu.o \
					arc4random.o posix_entropy.o
Darwin_defobjs    = $(all_posix_objs) darwin_cpu.o darwin_sem.o \
					darwin_clock.o
win32_defobjs     = $(all_win32_objs) win32_rand.o
OpenBSD_defobjs   = $(all_posix_objs) openbsd_cpu.o



#timerobjs = timer.o timer_int.o ctimer.o


baseobjs = mempool.o dirname.o \
           escape.o unescape.o mmap.o sysexception.o syserror.o \
		   getopt_long.o error.o jenkins_hash.o  \
		   bloom.o bloom_marshal.o str2hex.o frand.o fast-ht.o \
		   b64_encode.o b64_decode.o humanize.o strtosize.o \
		   cmutex.o arena.o memmgr.o hexdump.o \
		   hashtab.o  hashtab_iter.o strunquote.o \
		   readpass.o cmdline.o  uuid.o ulid.o \
		   hsieh_hash.o fnvhash.o murmur3_hash.o cityhash.o \
		   fasthash.o siphash24.o yorrike.o xorshift.o xxhash.o \
		   metrohash64.o metrohash128.o xoroshiro.o \
		   mkdirhier.o parse-ip.o strcopy.o \
		   gstring.o gstring_var.o freadline.o rotatefile.o \
		   strsplit.o strsplit_csv.o strtrim.o \
		   pack.o \
		   $($(platform)_objs)




default-libobjs = $(baseobjs) $($(platform)_defobjs)

# Only assign if top level GNUmakefile hasn't done anything
libobjs ?= $(default-libobjs)

# Do this in the beginning so that VPATH etc. gets initialized
INCDIRS = $($(platform)_incdirs)
VPATH 	= $($(platform)_vpath)

INCDIRS += .  $(PORTABLE)/inc
VPATH 	+= $(PORTABLE)/src

INCS	= $(addprefix -I,$(INCDIRS))
DEFS	= -D__$(platform)__=1 $($(platform)_defs)

# Cross compiling Win32 binaries on Linux. Way cool :-)
ifneq ($(CROSS),)


	mingw_ok    = $(shell if $(CROSS)gcc -S -o /dev/null -xc /dev/null >/dev/null 2>&1; then echo "yes"; else echo "no"; fi;)


	ifeq ($(mingw_ok),yes)
		CROSSPATH := $(CROSS)
		platform  := win32
	else

		# Check for debian cross compiler
		mingw_ok = $(shell if i586-mingw32msvc-gcc -S -o /dev/null -xc /dev/null >/dev/null 2>&1; then echo "yes"; else echo "no"; fi;)

		ifeq ($(mingw_ok),yes)
			CROSSPATH := i586-mingw32msvc-
			platform  := win32
		else
			junk := $(error ** $(CROSS) Cross compiler  is not available!)
		endif
	endif

else
	ifneq ($(cyg-cross),)
		CROSSPATH := $(cyg-cross)
	endif
endif


ifeq ($(platform),win32)
	exe := .exe
	WIN32 := 1
endif

# Tools we need
AR    = $(CROSSPATH)ar
CC    = $(CROSSPATH)gcc
CXX   = $(CROSSPATH)g++
LD    = $(CXX)
RANLIB = $(CROSSPATH)ranlib

# Aux tools we need
TOOLSDIR = $(PORTABLE)/tools
DEPWEED  = $(TOOLSDIR)/depweed.py
MKGETOPT = $(TOOLSDIR)/mkgetopt.py


WARNINGS	    = -Wall -Wextra -Wpointer-arith
EXTRA_WARNINGS  = -Wsign-compare -Wconversion

win32_CFLAGS   += -mno-cygwin -mwin32 -mthreads

Linux_CXXFLAGS += -std=c++11
Linux_ldlibs   += -lpthread

ARFLAGS  = rv
CFLAGS  += $($(platform)_CFLAGS) $(WARNINGS) $(INCS) $(DEFS)
LDFLAGS += $($(platform)_ldflags)


CXXFLAGS += $(CFLAGS) $($(platform)_CXXFLAGS)



# XXX Can use better optimization on newer CPUs
Linux_OFLAGS   = -fexpensive-optimizations
OpenBSD_OFLAGS = -fexpensive-optimizations
Darwin_OFLAGS  =

ifeq ($(OPTIMIZE),1)
	CFLAGS  += -O3 $($(platform)_OFLAGS) $(OFLAGS) \
			   -g -Wuninitialized -D__MAKE_OPTIMIZE__=1

	objdira := rel

# Strip the final executable
define strip-cmd
	$(CROSSPATH)strip $@
endef

	#CFLAGS += -ffunction-sections -fdata-sections

	ifneq ($(STATIC),)
		LDFLAGS += -Bstatic
	endif

else
	CFLAGS  += -g
	LDFLAGS += -g

	objdira := dbg

# no-op
define strip-cmd
	true
endef

endif


# vim: ft=make:sw=4:ts=4:noexpandtab:notextmode:tw=72:
