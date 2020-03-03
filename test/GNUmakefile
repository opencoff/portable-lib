#! /usr/bin/env make -f

# Platform specification
uname   := $(shell uname -s)
ifneq ($(OS),)
	WIN32 	 := 1
	platform := win32

	# Cygwin is detestable
	ifneq ($(findstring CYGWIN,$(uname)),)
		cyg-cross  := mingw32-
		cygwin := 1
	endif
else
    ifneq ($(findstring mingw, $(CROSS)),)
       WIN32    := 1
       platform := win32
    else
       platform := $(uname)
    endif
endif


CROSSPATH = /usr/bin/
SRCDIR    = .
PORTABLE := ..


# My test cases
posix_tests +=
Linux_tests  = $(posix_tests) mt-dd-wipe
Darwin_tests = $(posix_tests) mt-dd-wipe
win32_tests += mmap_win32 t_socketpair

#posix_tests += t_resolve
posix_tests += t_cresolve

# What tests to build
autotests = strmatch t_arena t_str2hex t_esc \
		t_ipaddr t_strcopy t_strtoi \
		t_pack t_strsplit_csv t_splitargs \
		t_strsplit t_gvarexp t_dirname t_uuid \
		t_frand t_ulid t_xorfilter \
		t_fixedsize t_mempool \
		t_list t_b64_c t_vect \
		t_xorrand  t_strtosize \
		t_bits t_siphash24 t_progress \
		t_spscq t_bitvect t_fts t_hashspeed \
		t_prodcons t_mpmcq t_ringbuf

tests_with_input = mmaptest t_mkdirhier  \
                   t_readpass t_rotatefile

hash-input = $(wildcard ./in) # input for all the tests below
hashtests = t_hashtab t_fast-ht t_hashbench t_bloom

tests = $(autotests) $(tests_with_input) $(hashtests) $($(platform)_tests)

nettests =

# List your target here
target = $(tests)

#CFLAGS += -mavx


define obj_def
$(1)_objs = $(1).o
endef

# Define objs for each test
$(foreach p,$(target),$(eval $(call obj_def,$(p))))

#$(warning targets=$(target))
#$(warning b64_objs=$(b64_objs))

mt-dd-wipe_opt_objs = dd-wipe-opt.o
mt-dd-wipe_objs := mt-dd-wipe.o disksize.o


# Define common library objects needed for this project
# PORTABLE must be defined above..
include $(PORTABLE)/portablelib.mk

# Set this up for profiling
#CFLAGS += -pg -no-pie
#LDFLAGS += -pg -no-pie
sanitize = -fbounds-check -fsanitize=address -fsanitize=leak -fsanitize=bounds
CFLAGS += -D_FORTIFY_SOURCE=2 $(sanitize)
LDFLAGS += $(sanitize)

# Include
include $(PORTABLE)/tools.mk

test:
	for t in $(autotests); do  \
        echo "TEST $$t:"; \
		./$(objdir)/$$t; \
    done || exit 1
ifneq ($(hash-input),)
	for t in $(hashtests); do \
		./$(objdir)/$$t $(hash-input) ; \
	done || exit 1
endif



#  DON'T DELETE BELOW!
# vim: filetype=make
# vim: tw=4:sw=4:noexpandtab:smartindent:tw=76:
