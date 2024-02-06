
# Common fragment for people using this library
# Usage in your GNUmakefile:
#
#  - define PORTABLE to path to this top level directory
#  - include $(PORTABLE)/portablelib.mk

ifeq ($(PORTABLE),)
	junk := $(error ** Please define PORTABLE before including this makefile **)
endif

ifeq ($(os),)
	os := $(shell uname -s | tr '[A-Z]' '[a-z]')
endif


_mach := $(shell uname -m)
mach_x86_64 := amd64
mach_aarch64 := aarch64
mach_arm := arm

ifneq ($(mach_$(_mach)),)
	machine := $(mach_$(_mach))
else
	machine := $(_mach)
endif

all_win32_objs += gettimeofday.o truncate.o ndir.o \
					ftruncate.o sleep.o basename.o dirname.o glob.o \
					winmmap.o errno.o fsync.o inetutils.o \
					socketpair.o

win32_vpath   += $(PORTABLE)/src/win32
win32_incdirs += $(PORTABLE)/inc/win32
win32_defs    += -DWIN32 -D_WIN32 -DWINVER=0x0501
win32_tests   +=
win32_LDFLAGS +=
win32_ldlibs  += -lwsock32

all_posix_objs = daemon.o

#all_posix_objs += resolve.o
all_posix_objs += c_resolve.o work.o job.o

posix_vpath    += $(PORTABLE)/src/posix
posix_incdirs  +=
posix_defs     +=
posix_tests    +=
posix_ldlibs   +=
posix_LDFLAGS  +=

linux_ldlibs     += -lsodium
linux_defs       += -D_GNU_SOURCE=1

darwin_incdirs   += /opt/local/include
darwin_ldlibs    += /opt/local/lib/libsodium.a

openbsd_incdirs  += /usr/local/include
openbsd_ldlibs	 += -L/usr/local/lib -lsodium -lpthread

DragonFly_incdirs += /usr/local/include /usr/local/include/ncurses
DragonFly_ldlibs  += -L/usr/local/lib -lsodium -lpthread

posix_oses  = linux darwin solaris openbsd freebsd netbsd dragonfly

# Helper for Posix OSes
ifneq ($(findstring $(os),$(posix_oses)),)
	$(os)_vpath   += $(posix_vpath)
	$(os)_incdirs += $(PORTABLE)/inc/$(os) $(posix_incdirs)
	$(os)_defs    += $(posix_defs)
	$(os)_LDFLAGS += $(posix_LDFLAGS)
	$(os)_ldlibs  += $(posix_ldlibs)
endif


# Default objs for linux 
linux_defobjs     = $(all_posix_objs) linux_cpu.o \
					arc4random.o posix_entropy.o
darwin_defobjs    = $(all_posix_objs) darwin_cpu.o darwin_sem.o
win32_defobjs     = $(all_win32_objs) win32_rand.o
openbsd_defobjs   = $(all_posix_objs) openbsd_cpu.o

# DragonFly BSD supports linux's sched_{get/set}affinity()
dragonfly_defobjs = $(all_posix_objs) nofdatasync.o linux_cpu.o



#timerobjs = timer.o timer_int.o ctimer.o


baseobjs = mempool.o dirname.o fts.o splitargs.o \
           escape.o unescape.o mmap.o sysexception.o syserror.o \
		   getopt_long.o error.o xorfilter.o xorfilter_marshal.o \
		   bloom.o bloom_marshal.o str2hex.o frand.o fast-ht.o \
		   b64_encode.o b64_decode.o humanize.o strtosize.o \
		   cmutex.o arena.o memmgr.o hexdump.o \
		   hashtab.o  hashtab_iter.o strunquote.o \
		   readpass.o uuid.o ulid.o \
		   hsieh_hash.o fnvhash.o murmur3_hash.o cityhash.o \
		   fasthash.o siphash24.o yorrike.o xorshift.o xxhash.o \
		   metrohash64.o metrohash128.o xoroshiro.o romu-rand.o \
		   mkdirhier.o parse-ip.o strcopy.o \
		   gstring.o gstring_var.o freadline.o rotatefile.o \
		   strsplit.o strsplit_csv.o strtrim.o \
		   pack.o progbar.o \
		   $($(os)_objs)




default-libobjs = $(baseobjs) $($(os)_defobjs)

# Only assign if top level GNUmakefile hasn't done anything
libobjs ?= $(default-libobjs)

# Do this in the beginning so that VPATH etc. gets initialized
INCDIRS = $($(os)_incdirs)
VPATH 	= $($(os)_vpath)

INCDIRS += .  $(PORTABLE)/inc
VPATH 	+= $(PORTABLE)/src

INCS	= $(addprefix -I,$(INCDIRS))
DEFS	= -D__$(os)__=1 -D__$(machine)__=1 $($(os)_defs)

# Cross compiling Win32 binaries on Linux. Way cool :-)
ifneq ($(CROSS),)


	mingw_ok    = $(shell if $(CROSS)gcc -S -o /dev/null -xc /dev/null >/dev/null 2>&1; then echo "yes"; else echo "no"; fi;)


	ifeq ($(mingw_ok),yes)
		CROSSPATH := $(CROSS)
		os  := win32
	else

		# Check for debian cross compiler
		mingw_ok = $(shell if i586-mingw32msvc-gcc -S -o /dev/null -xc /dev/null >/dev/null 2>&1; then echo "yes"; else echo "no"; fi;)

		ifeq ($(mingw_ok),yes)
			CROSSPATH := i586-mingw32msvc-
			os  := win32
		else
			junk := $(error ** $(CROSS) Cross compiler  is not available!)
		endif
	endif

else
	ifneq ($(cyg-cross),)
		CROSSPATH := $(cyg-cross)
	endif
endif


ifeq ($(os),win32)
	exe := .exe
	WIN32 := 1
endif

# Aux tools we need
TOOLSDIR = $(PORTABLE)/tools
DEPWEED  = $(TOOLSDIR)/depweed.py
MKGETOPT = $(TOOLSDIR)/mkgetopt.py


WARNINGS	    = -Wall -Wextra -Wpointer-arith -Wshadow
#EXTRA_WARNINGS  = -Wsign-compare -Wconversion -Wsign-conversion
LDFLAGS += $(EXTRA_WARNINGS)

win32_CFLAGS   += -mno-cygwin -mwin32 -mthreads

sanitize = -fsanitize=address \
		   -fsanitize=leak -fsanitize=bounds -fsanitize=signed-integer-overflow
linux_CC =  gcc
linux_CXX = g++
linux_LD = g++
linux_CFLAGS   += $(sanitize)
linux_CXXFLAGS += $(sanitize)
linux_LDFLAGS  += $(sanitize)

linux_CXXFLAGS += -std=c++17
linux_ldlibs   += -lpthread

openbsd_CXX     = clang++
openbsd_CC      = clang
openbsd_LD      = clang++
openbsd_CXXFLAGS += -std=c++17

darwin_CC = clang
darwin_CXX = clang++
darwin_LD = clang++
darwin_SANITIZE = -fsanitize=undefined -fsanitize=address -fsanitize=bounds -fsanitize=signed-integer-overflow
darwin_CXXFLAGS += -std=c++17 $(darwin_SANITIZE)
darwin_CFLAGS += $(darwin_SANITIZE)
darwin_LDFLAGS += $(darwin_SANITIZE)

ARFLAGS  = rv
CFLAGS  += $($(os)_CFLAGS) $(WARNINGS) $(EXTRA_WARNINGS) $(INCS) $(DEFS)
LDFLAGS += $($(os)_LDFLAGS)


CXXFLAGS += $(CFLAGS) $($(os)_CXXFLAGS)



# XXX Can use better optimization on newer CPUs
linux_OFLAGS   = -D_FORTIFY_SOURCE=2 -mharden-sls=all
openbsd_OFLAGS =
darwin_OFLAGS  =

ifeq ($(RELEASE),1)
	OPTIMIZE := 1
endif

ifeq ($(OPTIMIZE),1)
	# FORTIFY_SOURCE is a no-op when not optimizing
	CFLAGS  += -O3 $($(os)_OFLAGS) $(OFLAGS) \
			   -g -Wuninitialized -D__MAKE_OPTIMIZE__=1

	LDFLAGS += -g

	buildtype := rel

# Strip the final executable
define strip-cmd
	#$(CROSSPATH)strip $@
	true
endef

	#CFLAGS += -ffunction-sections -fdata-sections

	ifneq ($(STATIC),)
		LDFLAGS += -Bstatic
	endif

else
	CFLAGS  += -g
	LDFLAGS += -g

	buildtype := dbg

# no-op
define strip-cmd
	true
endef

endif

ifneq ($($(os)_CC),)
	CC := $($(os)_CC)
else
	CC = $(CROSSPATH)gcc
endif

ifneq ($($(os)_CXX),)
	CXX := $($(os)_CXX)
else
	CXX  = $(CROSSPATH)g++
endif

ifneq ($($(os)_LD),)
	LD := $($(os)_LD)
else
	LD  = $(CXX)
endif

# Tools we need
AR    = $(CROSSPATH)ar
RANLIB = $(CROSSPATH)ranlib


# vim: ft=make:sw=4:ts=4:noexpandtab:notextmode:tw=72:
