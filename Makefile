# --------------------------------------------------------------------------------------------------
# Makefile used to build the Telematics application framework.
# --------------------------------------------------------------------------------------------------

TARGETS := sa415m sa515m sa525m
UTILITIES := clean distclean

export LEGATO_RELATIVE_PATH := ../legato/legato-af
export TELAF_ROOT := $(CURDIR)
export TELAF_BUILD := $(CURDIR)/build
export LEGATO_ROOT := $(CURDIR)/../legato/legato-af
export LEGATO_BUILD := $(CURDIR)/../legato/legato-af/build
export GEN_FILE_CONTEXTS := $(CURDIR)/security/selinux/tools/generate_telaf_file_contexts.sh
export SELINUX_FILE_CONTEXTS := ${CURDIR}/security/selinux/sepolicy/files/file_contexts

export PKG_CONFIG_SYSROOT_DIR ?=

SE_FILES = $(shell find $(CURDIR)/security/selinux/sepolicy/ -name *.pp -type f)
SE_MODS = $(shell find $(CURDIR)/security/selinux/sepolicy/ -name tmp -type d)

# Sub-Makefile for TelAF Simulation, but we need to
# prevent 'simulation' target from affecting other targets.
ifneq ($(filter simula%,$(MAKECMDGOALS)),)
  include simulation/simulation.mk
endif

# SDK configurations
include config.mk

$(TARGETS):
	@ln -sf $(LEGATO_RELATIVE_PATH)/build ./build
	$(shell $(GEN_FILE_CONTEXTS))
	$(MAKE) --no-print-directory -C $(LEGATO_ROOT) $@ TELAF_ROOT=$(TELAF_ROOT)

$(UTILITIES):
	@$(MAKE) --no-print-directory -C $(LEGATO_ROOT) $@ TELAF_ROOT=$(TELAF_ROOT)
	@rm -rf $(TELAF_BUILD)
	@rm -fr $(SE_FILES) $(SE_MODS)
	@rm -f simulation/workstation/.check_done
