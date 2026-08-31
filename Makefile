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
export TELAF_PA_DEFAULT := $(CURDIR)/../telaf-pa-default
export TELAF_PA := $(CURDIR)/../telaf-pa

export GEN_FILE_CONTEXTS := $(CURDIR)/security/selinux/tools/generate_telaf_file_contexts.sh
export SELINUX_FILE_CONTEXTS := ${CURDIR}/security/selinux/sepolicy/files/file_contexts

export PKG_CONFIG_SYSROOT_DIR ?=
DGTOOL ?= dgtool-V2

SE_FILES = $(shell find $(CURDIR)/security/selinux/sepolicy/ -name *.pp -type f)
SE_MODS = $(shell find $(CURDIR)/security/selinux/sepolicy/ -name tmp -type d)

PA_BUILD_DIRS := $(TELAF_PA)/build $(TELAF_PA)/staging $(TELAF_PA_DEFAULT)/build $(TELAF_PA_DEFAULT)/staging

# Out-of-source CMake plugin(s) that depend on the TelAF/legato build output.
# These are built after the main legato build so that the component libraries
# they link against (e.g. libComponent_tafDIDDataAccessComp.so) already exist.
DIDINDB_PLUGIN_DIR := $(CURDIR)/apps/plugin/didStoreInDB

# $(call BUILD_CMAKE_PLUGIN,<plugin-source-dir>,<target>)
# Configures and builds a CMake plugin. The output lands in <plugin-source-dir>/build
# as documented in the plugin README (libTafPiDiagDIDInDB_shared_lib.so). TARGET and
# TELAF_INTERFACES are exported for the CMakeLists which reads them via $ENV{...}.
define BUILD_CMAKE_PLUGIN
	@echo "Building CMake plugin: $(1) (target $(2))"
	@mkdir -p $(1)/build
	cd $(1)/build && \
		TARGET=$(2) \
		TELAF_INTERFACES=$(TELAF_ROOT)/interfaces \
		cmake .. && \
		$(MAKE) --no-print-directory
endef

export SIMULATION_ROOT := $(wildcard $(CURDIR)/../telaf-simulation)
ifeq ($(SIMULATION_ROOT),)
  SIMULATION_ROOT := $(CURDIR)/simulation
endif
$(info simulation root path @ $(SIMULATION_ROOT))

# Sub-Makefile for TelAF Simulation, but we need to
# prevent 'simulation' target from affecting other targets.
ifneq ($(filter simula%,$(MAKECMDGOALS)),)
  $(info import the simulation build flow)
  include $(SIMULATION_ROOT)/simulation.mk
endif

# SDK configurations
include config.mk

default:
	@echo "Nothing to do, without any target"

$(TARGETS): TARGET=$@
$(TARGETS):
	@ln -sf $(LEGATO_RELATIVE_PATH)/build ./build
	$(shell $(GEN_FILE_CONTEXTS))
	$(call PREBUILD_PA,$(TARGET))
	$(MAKE) --no-print-directory -C $(TELAF_ROOT)/apps/tools/tafDiagGen -f dgtool.mk $(DGTOOL) DGTOOL_TARGET=$(TARGET)
	$(MAKE) --no-print-directory -C $(LEGATO_ROOT) $@ TELAF_ROOT=$(TELAF_ROOT)
ifneq ($(BUILD_FLAVOR),lxc)
	$(call BUILD_CMAKE_PLUGIN,$(DIDINDB_PLUGIN_DIR),$(TARGET))
else
	@echo "BUILD_FLAVOR=lxc: skipping CMake plugin build ($(DIDINDB_PLUGIN_DIR))"
endif

$(UTILITIES):
	@$(MAKE) --no-print-directory -C $(LEGATO_ROOT) $@ TELAF_ROOT=$(TELAF_ROOT)
	@$(MAKE) --no-print-directory -C $(TELAF_ROOT)/apps/tools/tafDiagGen -f dgtool.mk cleanall-venv
	@rm -rf $(TELAF_BUILD) $(PA_BUILD_DIRS)
	@rm -rf $(DIDINDB_PLUGIN_DIR)/build
	@rm -fr $(SE_FILES) $(SE_MODS)
	@rm -f simulation/workstation/.check_done

