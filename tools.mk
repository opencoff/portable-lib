# vim: filetype=make
# vim: tw=4:sw=4:noexpandtab:smartindent:tw=76:

# Win32 needs an .exe suffix
ifeq ($(exe),)
	realtargets = $(addprefix $(objdir)/, $(target))
else
	realtargets = $(addprefix $(objdir)/, $(addsuffix $(exe),$(target)))
endif



# obj files are kept in separate dir that is os specific
objdir := obj-$(os)-$(machine)-$(buildtype)

__mkdir := $(shell test -d $(objdir) || mkdir -p $(objdir))



_libobjs = $(addprefix $(objdir)/, $(libobjs))

# Template for linking.
# $(1)         => link target
# $(1)_objs    => Name of the variable corresponding to this target
# $($(1)_objs) => Value of the above variable
#
# We need to add an extra '$' in the rules since it gets evaluated _twice_
# -- once at rule definition time and once when it is used.
#
# At the same time, we collect _all_ the objects (for all targets).
# We use $(all-objs) for generating dependencies.
define link_template

real_$(1)_objs := $$(addprefix $(objdir)/, $$($(1)_objs))
real_$(1)_opts := $$(addprefix $(objdir)/, $$($(1)_opt_objs))
real_$(1)_exe  := $$(addprefix $(objdir)/, $(1)$(exe))
all-objs	   += $$(real_$(1)_objs) $$(real_$(1)_opts)

$$(real_$(1)_objs): $$(real_$(1)_opts)
$$(real_$(1)_exe): $$(real_$(1)_objs) $$(real_$(1)_opts) $$(libs)

.PRECIOUS: $($$($(1)_opt_objs):.o=.c) .s .S
endef

deps		= $(all-objs:.o=.d) $(_libobjs:.o=.d)
libs		= $(addprefix $(objdir)/, utils.a)


SUFFIXES += .c .i .s .cpp .h .o

.PHONY: all clean realclean $(objdir)


all: $(realtargets)

$(realtargets):

# This defines a rule dynamically for linking each target
# named in $(target). It uses the template 'link_template'
# to do the dirty work.
$(foreach prog,$(target),$(eval $(call link_template,$(prog))))


#$(warning realtargets=$(realtargets))
#$(warning real_rechash_objs=$(real_rechash_objs))
#$(warning real_rechash_opts=$(real_rechash_opts))
#$(warning real_rechash_exe=$(real_rechash_exe))
#$(warning all-objs=$(all-objs))
#$(warning deps=$(deps))
#$(warning libobjs=$(libobjs))

# Single rule for all linkable targets
# XXX We implicitly use the C++ linker even if there are no C++ objs.
#     I don't know how to pick the C++ linker only if there are C++ objs.
$(realtargets):
	@echo $@
	$(LD) $(LDFLAGS) $^ -o $@ $($(os)_ldlibs) $($(notdir $(basename $@))_LIBS)
	$(strip-cmd)



# Common library used by _all_ objects
$(libs): $(_libobjs)
	-rm -f $@
	$(AR) $(ARFLAGS) $@ $^


objs: $(all-objs) $(_libobjs)

libobjs: $(_libobjs)
libs: $(libs)
phony:

clean:
	rm -f $(all-objs) $(_libobjs) $(realtargets) $(libs)

realclean: clean
	rm -f $(deps)

depclean: phony
	rm -f $(deps)


depend dep deps: $(deps)

date    := $(shell date)
ifneq ($(wildcard version),)

version := $(shell cat version)

#$($(target)_objs): version.h
$(addprefix $(objdir)/, $($(target)_objs)): version.h

version.h: version
	echo "#define VERSION_STR \"$(version)\"" > $@
	echo "#define BUILD_DATE  \"$(date)\"" >> $@

else
version := "*-none-*"

version.h: phony
	echo "#define VERSION_STR \"$(version)\"" > $@
	echo "#define BUILD_DATE  \"$(date)\"" >> $@
endif

echo:
	@echo "VPATH=$(VPATH)"
	@echo "CFLAGS=$(CFLAGS)"
	@echo "CXXFLAGS=$(CXXFLAGS)"


# Often, it is useful to be able to look at CPP output
%.i: %.c
	$(CC) -E -dMD $(CFLAGS) $< > $@

%.i: %.cpp
	$(CXX) -E -dMD $(CXXFLAGS) $< > $@


%.c %.h: %.in $(MKGETOPT)
	$(MKGETOPT) $<


#_MP := -MP

%.o: %.c
	$(CC) -MMD $(_MP) -MT '$@ $(@:.o=.d)' -MF "$(@:.o=.d)" $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CXX) -MMD $(_MP) -MT '$@ $(@:.o=.d)' -MF "$(@:.o=.d)" $(CXXFLAGS) -c -o $@ $<

%.S: %.cpp
	$(CXX) -MMD $(_MP) -MT '$@ $(@:.S=.d)' -MF "$(@:.o=.d)" $(CXXFLAGS) -S -o- $< | c++filt > $@



# Rules dependeng on objdir

#_MP := -MP

$(objdir)/%.o: %.c
	@echo $<
	$(CC) -MMD $(_MP) -MT '$@ $(@:.o=.d)' -MF "$(@:.o=.d)" $(CFLAGS) $($(notdir $@)_CFLAGS) -c -o $@ $<

$(objdir)/%.o: %.cpp
	$(CXX) -MMD $(_MP) -MT '$@ $(@:.o=.d)' -MF "$(@:.o=.d)" $(CXXFLAGS) $($(notdir $@)_CFLAGS) -c -o $@ $<

$(objdir)/%.o: %.cc
	$(CXX) -MMD $(_MP) -MT '$@ $(@:.o=.d)' -MF "$(@:.o=.d)" $(CXXFLAGS) $($(notdir $@)_CFLAGS) -c -o $@ $<

$(objdir)/%.i: %.c
	$(CC) -dMD $(CFLAGS) $($(notdir $@)_CFLAGS) -E $< > $@

$(objdir)/%.i: %.cpp
	$(CXX) -MMD $(_MP) -MT '$@ $(@:.S=.d)' -MF "$(@:.o=.d)" $(CFLAGS) $($(notdir $@)_CFLAGS) -E $< > $@


$(objdir)/%.S: %.c
	$(CC) -MMD $(_MP) -MT '$@ $(@:.S=.d)' -MF "$(@:.o=.d)" $(CFLAGS) $($(notdir $@)_CFLAGS) -S -o $@ $<

# Magic to include dependencies only during normal builds. If the
# make target is '*clean' (i.e., any variant of clean), we don't
# include the dependencies.
#$(warning "MAKECMDGOALS=$(MAKECMDGOALS)")
ifneq ($(findstring clean, $(MAKECMDGOALS)),clean)
-include $(shell $(DEPWEED) $(deps))
endif


