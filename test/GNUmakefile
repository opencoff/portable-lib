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
tests = strmatch t_strtoi t_arena t_str2hex \
		t_strsplit t_gvarexp t_dirname t_uuid \
		mmaptest t_mkdirhier t_frand t_ulid \
		t_fixedsize t_mempool t_opt_test \
		t_hashtab t_list t_b64_c t_vect \
		t_hashfunc t_xorrand t_fast-ht \
		t_hashspeed t_hashbench t_strtosize \
		t_bits t_siphash24 hashtok t_readpass \
		t_spscq t_mpmcq t_ipaddr t_strcopy \
		t_bloom t_bitvect  t_fts t_rotatefile \
		t_pack \
		$($(platform)_tests)


nettests =

# List your target here
target = $(tests)



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


# Include 
include $(PORTABLE)/tools.mk

tests:
	@echo $(target)




#  DON'T DELETE BELOW!
# vim: filetype=make
# vim: tw=4:sw=4:noexpandtab:smartindent:tw=76:
