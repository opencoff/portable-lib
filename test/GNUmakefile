#! /usr/bin/env make -f

CROSSPATH = /usr/bin/
SRCDIR    = .
PORTABLE := ..

# Define common library objects needed for this project
# PORTABLE must be defined above..
include $(PORTABLE)/portablelib.mk

# My test cases
posix_tests +=
#posix_tests += t_resolve
posix_tests += t_cresolve

# What tests to build
autotests = strmatch t_arena t_str2hex t_esc \
		t_ipaddr t_strcopy t_strtoi \
		t_pack t_strsplit_csv t_splitargs \
		t_strsplit t_gvarexp t_dirname t_uuid \
		t_list t_b64_c t_vect \
		t_xorrand  t_strtosize \
		t_bitvect t_fts \
		t_bits t_siphash24 t_progress \
		t_frand t_ulid t_hashspeed \
		t_xorfilter t_fixedsize t_mempool \
		t_spscq t_prodcons t_mpmcq t_ringbuf t_fast-ht-basic

tests_with_input = mmaptest t_mkdirhier  \
                   t_readpass t_rotatefile

hash-input = $(wildcard ./in*) # input for all the tests below
hashtests = t_hashtab t_fast-ht t_hashbench t_bloom t_xorfilter-file t_top1m

tests = $(autotests) $(tests_with_input) $(hashtests) $($(os)_tests)

nettests =

# List your target here
target = $(tests)

#CFLAGS += -mavx

linux_tests  += $(posix_tests) mt-dd-wipe
darwin_tests += $(posix_tests) mt-dd-wipe
win32_tests += mmap_win32 t_socketpair
dragonfly_tests += $(posix_tests)

linux_sanitize_tests = autotests

# XXX Sanitize doesn't work on Darwin-arm64!
darwin_sanitize_tests =

sanitize_tests = $($(os)_sanitize_tests)


define obj_def
$(1)_objs = $(1).o
endef

# Define objs for each test
$(foreach p,$(target),$(eval $(call obj_def,$(p))))

#$(warning targets=$(target))
#$(warning b64_objs=$(b64_objs))

mt-dd-wipe_opt_objs = dd-wipe-opt.o
mt-dd-wipe_objs := mt-dd-wipe.o disksize.o



# Include
include $(PORTABLE)/tools.mk

test: easytest hashtest sanitize

sanitize:
	@echo "bins in ./$(objdir)/ for sanitize .."; \
	for t in $(sanitize_tests); do  \
        echo "TEST $$t:"; \
	     ASAN_OPTIONS="detect_leaks=1" ./$(objdir)/$$t || exit 1; \
    done || exit 1

easytest:
	@echo "bins in ./$(objdir)/ for simple-tests .."; \
	for t in $(autotests); do  \
        echo "TEST $$t:"; \
		./$(objdir)/$$t || exit 1; \
    done || exit 1

hashtest:
ifneq ($(hash-input),)
	@echo "bins in ./$(objdir)/ for hash-tests .."; \
	for t in $(hashtests); do \
		echo "HASH-TEST $$t:"; \
		./$(objdir)/$$t $(hash-input) ; \
	done || exit 1
else
	true
endif


#  DON'T DELETE BELOW!
# vim: filetype=make
# vim: tw=4:sw=4:noexpandtab:smartindent:tw=76:
