# --------------------------------------------------------------------------------------------------
# Makefile used to build the Telematics application framework.
# --------------------------------------------------------------------------------------------------

TARGETS := sa415m sa515m
UTILITIES := clean distclean

export TELAF_ROOT := $(CURDIR)
export TELAF_BUILD := $(CURDIR)/build
export LEGATO_ROOT := $(CURDIR)/../legato/legato-af
export LEGATO_BUILD := $(CURDIR)/../legato/legato-af/build

$(TARGETS):
ifneq ($(TELAF_BUILD), $(wildcard $(TELAF_BUILD)))
	@ln -sf $(LEGATO_BUILD) $(TELAF_BUILD)
endif
	$(MAKE) -C $(LEGATO_ROOT) $@ TELAF_ROOT_SET=$(TELAF_ROOT)

$(UTILITIES):
	$(MAKE) -C $(LEGATO_ROOT) $@ TELAF_ROOT_SET=$(TELAF_ROOT)
ifeq ($(TELAF_BUILD), $(wildcard $(TELAF_BUILD)))
	@rm -rf $(TELAF_BUILD)
endif

