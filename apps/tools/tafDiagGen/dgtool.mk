# ---------------------------------
# Diagnositc gen tool main Makefile
# ---------------------------------

.PHONY: dgtool

dgtool: dgtool-V2
dgtool-setup: dgtool-V2-setup
dgtool-build: dgtool-V2-build
dgtool-install:dgtool-V2-install
dgtool-clean: dgtool-V2-clean

# Not case-sensitive
dgtool-v1%: dgtool-V1%
	@:
dgtool-v2%: dgtool-V2%
	@:

# !! We didn't intend to use advanced syntax for Makefile, but made sure the file was readable!

# --- Version 1 ---

dgtool-V1: dgtool-V1-setup dgtool-V1-build dgtool-V1-install
	@echo "[$@] <-- Done"

dgtool-V1-setup:
	@echo "[$@] <--"
	@$(MAKE) --no-print-directory -C $(TELAF_ROOT)/apps/tools/tafDiagGen/$(word 2, $(subst -, ,$@))/ setup

dgtool-V1-build:
	@echo "[$@] <--"
	@$(MAKE) --no-print-directory -C $(TELAF_ROOT)/apps/tools/tafDiagGen/$(word 2, $(subst -, ,$@))/ default

dgtool-V1-install:
	@echo "[$@] <--"
	@$(MAKE) --no-print-directory -C $(TELAF_ROOT)/apps/tools/tafDiagGen/$(word 2, $(subst -, ,$@))/ install

dgtool-V1-clean:
	@echo "[$@] <--"
	@$(MAKE) --no-print-directory -C $(TELAF_ROOT)/apps/tools/tafDiagGen/$(word 2, $(subst -, ,$@))/ clean


# --- Version 2 ---

dgtool-V2: dgtool-V2-setup dgtool-V2-build dgtool-V2-install
	@echo "[$@] <-- Done"

dgtool-V2-setup:
	@echo "[$@] <--"
	@$(MAKE) --no-print-directory -C $(TELAF_ROOT)/apps/tools/tafDiagGen/$(word 2, $(subst -, ,$@))/ setup

dgtool-V2-build:
	@echo "[$@] <--"
	@$(MAKE) --no-print-directory -C $(TELAF_ROOT)/apps/tools/tafDiagGen/$(word 2, $(subst -, ,$@))/ default

dgtool-V2-install:
	@echo "[$@] <--"
	@$(MAKE) --no-print-directory -C $(TELAF_ROOT)/apps/tools/tafDiagGen/$(word 2, $(subst -, ,$@))/ install

dgtool-V2-clean:
	@echo "[$@] <--"
	@$(MAKE) --no-print-directory -C $(TELAF_ROOT)/apps/tools/tafDiagGen/$(word 2, $(subst -, ,$@))/ clean
