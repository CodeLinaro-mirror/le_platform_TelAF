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

# Sub-Makefile for TelAF Simulation, but we need to
# prevent 'simulation' target from affecting other targets.
ifneq ($(filter simula%,$(MAKECMDGOALS)),)
  include simulation/simulation.mk
endif

# SDK configurations
include config.mk

default:
	@echo "Nothing to do, without any target"

# No PA for the LXC contianer
ifneq ($(BUILD_FLAVOR),lxc)
# Macro to check and copy stub directories
define PREBUILD_PA
	@echo "Finding and creating stub PA.."
	@find $(TELAF_ROOT) -name '.ssh' -prune -o -name Component.cdef | while read -r cdef_file; do \
		base_name=""; \
		while IFS= read -r line; do \
			if echo "$$line" | grep -q '$$LEGATO_BUILD/stub/component/'; then \
				base_name=$$(echo "$$line" | sed -n 's|.*$$LEGATO_BUILD/stub/component/\([^ ]*\).*|\1|p'); \
				echo "Required stub PA: $$base_name"; \
				target_stub_dir=$(LEGATO_RELATIVE_PATH)/build/$(1)/stub/component/$$base_name; \
				if [ ! -d "$$target_stub_dir" ]; then \
					mkdir -p $$target_stub_dir; \
				else \
					echo "PA folder is created: $$target_stub_dir"; \
				fi; \
				stub_dir=$(TELAF_PA_DEFAULT)/component/taf_pa_stub; \
				echo "Copy $$stub_dir to $$target_stub_dir"; \
				cp -r $$stub_dir/* $$target_stub_dir; \
			fi; \
		done < "$$cdef_file"; \
	done
endef
endif # ($(BUILD_FLAVOR),lxc)


$(TARGETS): TARGET=$@
$(TARGETS):
	@ln -sf $(LEGATO_RELATIVE_PATH)/build ./build
	$(shell $(GEN_FILE_CONTEXTS))
	$(call PREBUILD_PA,$(TARGET))
# $(MAKE) --no-print-directory -C $(TELAF_ROOT)/apps/tools/tafDiagGen -f dgtool.mk $(DGTOOL) DGTOOL_TARGET=$(TARGET)
	$(MAKE) --no-print-directory -C $(LEGATO_ROOT) $@ TELAF_ROOT=$(TELAF_ROOT)

$(UTILITIES):
	@$(MAKE) --no-print-directory -C $(LEGATO_ROOT) $@ TELAF_ROOT=$(TELAF_ROOT)
# @$(MAKE) --no-print-directory -C $(TELAF_ROOT)/apps/tools/tafDiagGen -f dgtool.mk cleanall-venv
	@rm -rf $(TELAF_BUILD)
	@rm -fr $(SE_FILES) $(SE_MODS)
	@rm -f simulation/workstation/.check_done

