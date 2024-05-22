
default:
	@echo "usage:"
	@echo "  update -- update repos based on patch_me.json files"

u update:
	@echo "Re-preparation all repos..."
	@python3 helpme_setup.py

.PHONY: sdk
sdk:
	@echo "Quick compile sdk simulation parts"
	@cd sdk && ./build_sim.sh setup && ./build_sim.sh all

clean-sdk:
	@echo "Clean sdk project"
	@cd sdk && ./build_sim.sh clean

distclean-sdk:
	@echo "Dist clean sdk project"
	@cd sdk && rm -rf build/ rootfs/
