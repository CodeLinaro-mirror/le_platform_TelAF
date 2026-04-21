# ---------------------------------
# Diagnositc gen tool main Makefile
# ---------------------------------

.PHONY: dgtool

default:
	@echo "Nothing to do for dgtool."

dgtool: dgtool-V2
dgtool-setup: dgtool-V2-setup
dgtool-build: dgtool-V2-build
dgtool-install:dgtool-V2-install
dgtool-clean: dgtool-V2-clean

export DGTOOL_TARGET ?= none

# Not case-sensitive
dgtool-v1: dgtool-V1
dgtool-v2: dgtool-V2

dgtool-v1%: dgtool-V1%
	@:
dgtool-v2%: dgtool-V2%
	@:

DGTOOL_PY ?= /usr/bin/python3

# Must use '/usr/bin/python3' to specify the HOST python environment
$(TELAF_ROOT)/apps/tools/tafDiagGen/venv/$(DGTOOL_TARGET):
	@echo "Create the python virtual envionment for dgtool"
	@mkdir -p $(TELAF_ROOT)/apps/tools/tafDiagGen/venv/$(DGTOOL_TARGET)
	$(DGTOOL_PY) -m venv --system-site-packages $(TELAF_ROOT)/apps/tools/tafDiagGen/venv/$(DGTOOL_TARGET)

cleanall-venv:
	@echo "Clean the python virtual environment for dgtool"
	rm -rf $(TELAF_ROOT)/apps/tools/tafDiagGen/venv
	rm -rf $(TELAF_ROOT)/components/tafDiagSvc/tafDiagCfg/serialization.host

# !! We didn't intend to use advanced syntax for Makefile, but made sure the file was readable!

# --- Version 1 ---

dgtool-V1: dgtool-V1-setup dgtool-V1-build dgtool-V1-install dgtool-V1-serialize
	@echo "[$@] <-- Done"

dgtool-V1-setup: $(TELAF_ROOT)/apps/tools/tafDiagGen/venv/$(DGTOOL_TARGET)
	@echo "[$@] <--"
	@$(MAKE) --no-print-directory -C V1 setup

dgtool-V1-build:
	@echo "[$@] <--"
	@$(MAKE) --no-print-directory -C V1 default

dgtool-V1-install:
	@echo "[$@] <--"
	@$(MAKE) --no-print-directory -C V1 install

dgtool-V1-serialize:
	@echo "[$@] <--"
	@$(MAKE) --no-print-directory -C V1 serialize

dgtool-V1-clean:
	@echo "[$@] <--"
	@$(MAKE) --no-print-directory -C V1 clean


# --- Version 2 ---

dgtool-V2: dgtool-V2-setup dgtool-V2-build dgtool-V2-install dgtool-V2-serialize
	@echo "[$@] <-- Done"

dgtool-V2-setup: $(TELAF_ROOT)/apps/tools/tafDiagGen/venv/$(DGTOOL_TARGET)
	@echo "[$@] <--"
	@$(MAKE) --no-print-directory -C V2 setup

dgtool-V2-build:
	@echo "[$@] <--"
	@$(MAKE) --no-print-directory -C V2 default

dgtool-V2-install:
	@echo "[$@] <--"
	@$(MAKE) --no-print-directory -C V2 install

dgtool-V2-serialize:
	@echo "[$@] <--"
	@$(MAKE) --no-print-directory -C V2 serialize

dgtool-V2-clean:
	@echo "[$@] <--"
	@$(MAKE) --no-print-directory -C V2 clean
