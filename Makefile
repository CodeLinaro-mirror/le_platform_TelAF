# --------------------------------------------------------------------------------------------------
# Makefile used to build the Telematics application framework.
# --------------------------------------------------------------------------------------------------

TARGETS := sa415m sa515m
UTILITIES := clean distclean

export LEGATO_RELATIVE_PATH := ../legato/legato-af
export TELAF_ROOT := $(CURDIR)
export TELAF_BUILD := $(CURDIR)/build
export LEGATO_ROOT := $(CURDIR)/../legato/legato-af
export LEGATO_BUILD := $(CURDIR)/../legato/legato-af/build
export GEN_FILE_CONTEXTS := $(CURDIR)/security/selinux/tools/generate_telaf_file_contexts.sh
export SELINUX_FILE_CONTEXTS := ${CURDIR}/security/selinux/sepolicy/files/file_contexts

$(TARGETS):
ifneq ($(TELAF_BUILD), $(wildcard $(TELAF_BUILD)))
	@ln -sf $(LEGATO_RELATIVE_PATH)/build ./build
endif
	$(shell $(GEN_FILE_CONTEXTS))
	$(MAKE) -C $(LEGATO_ROOT) $@ TELAF_ROOT_SET=$(TELAF_ROOT)

$(UTILITIES):
	$(MAKE) -C $(LEGATO_ROOT) $@ TELAF_ROOT_SET=$(TELAF_ROOT)
ifeq ($(TELAF_BUILD), $(wildcard $(TELAF_BUILD)))
	@rm -rf $(TELAF_BUILD)
endif

