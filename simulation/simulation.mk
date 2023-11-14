# --------------------------------------------------------------------------------------------------
# Makefile for TelAF Simulation Target
# --------------------------------------------------------------------------------------------------

TARGETS += simulation

Q?=@

# If you want make a compilation in your docker container, get along with below.
export within ?=

ifeq ($(lastword $(MAKECMDGOALS)),simulation)
$(error Please pass 'simula' for TelAF Simulation Target [simulation])
endif

# For embedded target, the cross-compilation tool will change the 'sysroot'
# to search header & libraries that have beed relocated.
# when we get the path by '--print-sysroot', that value will be returned.
# But for simulation target, the default prefix (empty) is used,
# because the host default environment for the gcc compiler is that.
# So for consistency, we add the required path without any affect for mktools.
export SYSROOT=/

export SIMULATION_HOME := $(CURDIR)/simulation
export SIMULATION_DEPS_INSTALL := $(SIMULATION_HOME)/deps/install
export SIMULATION_DEPS_SOURCE := $(SIMULATION_HOME)/deps/source
export SIMULATION_SCRIPTS := $(SIMULATION_HOME)/scripts
export SIMULATION_WORKDIR := $(SIMULATION_HOME)/workstation
SIMULATION_DEPS += # Empty is default, but it is post-extended
SIMULATION_TARBALL := $(SIMULATION_HOME)/workstation/telaf_simulation.tar

# Another way:
# 1. mkdir $(SIMULATION_WORKDIR)/sdk_rootfs
# 2. sudo mount --bind /path/to/sdk_rootfs  $(SIMULATION_WORKDIR)/sdk_rootfs
export sdk_rootfs ?= $(SIMULATION_WORKDIR)/sdk_rootfs
CHECK_SDK_ROOTFS := $(shell if [ -d "$(sdk_rootfs)" ]; then echo "y"; else echo "n"; fi)

export IMPORT_SDK_SIMULATION ?= n

ifneq ($(IMPORT_SDK_SIMULATION),n)

ifeq ($(CHECK_SDK_ROOTFS),n)
$(error sdk rootfs path is invalid [$(sdk_rootfs)], please check it)
else
$(warning sdk rootfs path [$(sdk_rootfs)])
endif

MKTOOLS_FLAGS_SIMULATION_EX += --cxxflags=-I$(sdk_rootfs)/include --ldflags=-L$(sdk_rootfs)/lib

export TELAF_SIMULATION_ENABLE_SMS ?= n
export TELAF_SIMULATION_ENABLE_DCS ?= n

endif

export TELAF_SIMULATION_ENABLE_SOMEIP_GW ?= y

SIMULATION_SOMEIP_GW_DEPS_y := $(SIMULATION_HOME)/deps/install/boost $(SIMULATION_HOME)/deps/install/vsomeip
SIMULATION_DEPS += $(SIMULATION_SOMEIP_GW_DEPS_$(TELAF_SIMULATION_ENABLE_SOMEIP_GW))

MKTOOLS_FLAGS_SIMULATION_EX += -X -std=c++11 -X -lstdc++

ifneq ($(TELAF_SIMULATION_ENABLE_SOMEIP_GW),n)
MKTOOLS_FLAGS_SIMULATION_EX += --cxxflags=-I$(TELAF_ROOT)/simulation/deps/install/boost/include \
                             --cxxflags=-I$(TELAF_ROOT)/simulation/deps/install/vsomeip/include \
                             --ldflags=-L$(TELAF_ROOT)/simulation/deps/install/boost/lib \
                             --ldflags=-L$(TELAF_ROOT)/simulation/deps/install/vsomeip/lib
endif

export MKTOOLS_FLAGS_SIMULATION_EX

.PHONY: simulation boost vsomeip

ifeq ($(within),)
simula simulac simula-c: check-sys pre-simulation-build simulation post-simulation-build
else
simula simulac simula-c: simula-up-develop-for-c
endif

OS_VERSION=$(shell grep -oP 'VERSION_ID=\K"(.+)"' /etc/os-release | tr -d '"')

check-sys:
	@echo "TelAF Simulation pre-checking your system ..."
	@if [ -e $(SIMULATION_WORKDIR)/.check_done ] && [ "`umask`" = "0022" ]; then \
		echo "Current system is already checked [OK]"; \
	else \
		if /bin/bash $(SIMULATION_SCRIPTS)/check_sys.sh ; then \
			echo "Current system is [OK]"; \
			echo -n "from $(OS_VERSION)" > $(SIMULATION_WORKDIR)/.check_done; \
		else \
			if [ -e $(SIMULATION_WORKDIR)/.check_done ]; then rm $(SIMULATION_WORKDIR)/.check_done; fi ; \
			echo "Current system is [NOK], please check above log for details."; exit 1 ; \
		fi \
	fi

pre-simulation-build: $(SIMULATION_HOME)/workstation/up_simulation.sh $(SIMULATION_DEPS:%=%/lib)

$(SIMULATION_SOMEIP_GW_DEPS_y:%=%/lib): simula-vsomeip

post-simulation-build: CURRENT_SYSTEM_OUTPUT=$(TELAF_BUILD)/simulation/_staging_system.simulation.update_ro/systems/current
post-simulation-build:
	@echo "[Simulation]: Creating Tarball ..."
ifneq ($(CHECK_SDK_ROOTFS),n)
	@mkdir -p $(CURRENT_SYSTEM_OUTPUT)/sdk_rootfs
	@cp -a $(sdk_rootfs)/* $(CURRENT_SYSTEM_OUTPUT)/sdk_rootfs
endif
	@tar cf $(SIMULATION_TARBALL) -C $(TELAF_BUILD)/simulation/_staging_system.simulation.update_ro .
	@tar rf $(SIMULATION_TARBALL) -C $(SIMULATION_HOME)/workstation/ up_simulation.sh
	@tar rf $(SIMULATION_TARBALL) -C $(SIMULATION_HOME)/workstation/ .check_done
ifneq ($(TELAF_SIMULATION_ENABLE_SOMEIP_GW),n)
	@tar rf $(SIMULATION_TARBALL) --exclude=install/boost/include \
	                              --exclude=install/boost/lib/cmake \
	                              --exclude=install/vsomeip/include \
	                              --exclude=install/vsomeip/etc \
	                              --exclude=install/vsomeip/lib/cmake \
	                              --exclude=install/vsomeip/lib/pkgconfig \
	                              -C $(SIMULATION_HOME)/deps/ install
endif
	@gzip -f $(SIMULATION_TARBALL)
	@echo "[Simulation]: Tarball $(SIMULATION_TARBALL).gz done."


BOOST_URL?=https://boostorg.jfrog.io/artifactory/main/release/1.74.0/source/boost_1_74_0.tar.gz
_BOOST_VERSION=$(notdir $(lastword $(subst /, ,$(BOOST_URL))))
BOOST_VERSION=$(_BOOST_VERSION:%.tar.gz=%)

VSOMEIP_URL?=https://github.com/COVESA/vsomeip/archive/refs/tags/3.1.20.3.tar.gz
_VSOMEIP_VERSION=$(notdir $(lastword $(subst /, ,$(VSOMEIP_URL))))
VSOMEIP_VERSION=$(_VSOMEIP_VERSION:%.tar.gz=vsomeip-%)

define setup-simulation-dep
	@if ! [ -e $(SIMULATION_DEPS_SOURCE)/.$(1).status ]; then \
		$(MAKE) --no-print-directory $(1)_download $(1)_build $(1)_install ; \
	elif [ "`cat $(SIMULATION_DEPS_SOURCE)/.$(1).status`" = "Inited" ]; then \
		$(MAKE) --no-print-directory $(1)_build $(1)_install ; \
	elif [ "`cat $(SIMULATION_DEPS_SOURCE)/.$(1).status`" = "Compiled" ]; then \
		$(MAKE) --no-print-directory $(1)_install ; \
	else \
		echo "[$(1)] Ready" ; \
	fi
endef

simula-boost: boost
boost:
	$(call setup-simulation-dep,$@)

simula-boost-download: boost_download
boost_download:
	@echo "[$@] downloading from [$(BOOST_URL)]" && wget -q -O \
		$(SIMULATION_DEPS_SOURCE)/$(_BOOST_VERSION) $(BOOST_URL) > /dev/null
	@echo "[$@] extract to [$(SIMULATION_DEPS_SOURCE)/boost]" && cd $(SIMULATION_DEPS_SOURCE) \
		&& if [ -d boost ]; then rm -r boost; fi \
		&& tar xfz $(_BOOST_VERSION) && mv $(BOOST_VERSION) boost
	@echo -n "Inited" > $(SIMULATION_DEPS_SOURCE)/.boost.status
	@echo "[$@] to Inited"

simula-boost-build: boost_build
boost_build: $(SIMULATION_DEPS_SOURCE)/boost
	@if ! [ -e $(SIMULATION_DEPS_SOURCE)/.boost.status ]; then \
		echo "[$@] You MUST download source-code, try: boost_download as target" ; exit 1 ; fi
	@if ! [ -d $(SIMULATION_DEPS_INSTALL)/boost ]; then \
		mkdir -p $(SIMULATION_DEPS_INSTALL)/boost ; fi
	@cd $(SIMULATION_DEPS_SOURCE)/boost \
		&& echo "[$@] configure boost firstly" \
		&& ./bootstrap.sh --prefix=$(SIMULATION_DEPS_INSTALL)/boost > /dev/null \
		&& echo "[$@] compiling boost ..."  \
		&& ./b2 > /dev/null
	@echo -n "Compiled" > $(SIMULATION_DEPS_SOURCE)/.boost.status
	@echo "[$@] to Compiled"

simula-boost-install: boost_install
boost_install: $(SIMULATION_DEPS_SOURCE)/boost/stage
	@if ! [ -e $(SIMULATION_DEPS_SOURCE)/.boost.status ] || [ "`cat $(SIMULATION_DEPS_SOURCE)/.boost.status`" = "Inited" ]; then \
		echo "[$@] You MUST build source-code, try: boost_build as target" ; exit 1 ; fi
	@cd $(SIMULATION_DEPS_SOURCE)/boost \
		&& if [ -d $(SIMULATION_DEPS_INSTALL)/boost ]; then \
			echo "[$@] remove old install" && rm -r $(SIMULATION_DEPS_INSTALL)/boost; fi \
		&& echo "[$@] installing ..." \
		&& mkdir -p $(SIMULATION_DEPS_INSTALL)/boost \
		&& ./b2 install > /dev/null
	@echo -n "Installed" > $(SIMULATION_DEPS_SOURCE)/.boost.status
	@echo "[$@] to Installed"

simula-boost-uninstall: boost_uninstall
boost_uninstall: $(SIMULATION_DEPS_SOURCE)/boost/stage
	@echo "[$@] un-installing"
	@rm -rf $(SIMULATION_DEPS_INSTALL)/boost
	@echo -n "Compiled" > $(SIMULATION_DEPS_SOURCE)/.boost.status
	@echo "[$@] to Compiled"

simula-boost-clean: boost_clean
boost_clean: $(SIMULATION_DEPS_SOURCE)/boost/stage
	@cd $(SIMULATION_DEPS_SOURCE)/boost \
		&& echo "[$@] cleaning ..." \
		&& ./b2 clean > /dev/null
	@echo -n "Inited" > $(SIMULATION_DEPS_SOURCE)/.boost.status
	@echo "[$@] to Inited"

simula-boost-remove boost_remove: boost_remove_install boost_remove_source

simula-boost-remove-install: boost_remove_install
boost_remove_install:
	@echo "[$@] removing install ..."
	@rm -rf $(SIMULATION_DEPS_INSTALL)/boost
	@echo "[$@] done"

simula-boost-remove-source: boost_remove_source
boost_remove_source:
	@echo "[$@] removing source & .boost.status file ..."
	@rm -rf $(SIMULATION_DEPS_SOURCE)/boost
	-@rm -f $(SIMULATION_DEPS_SOURCE)/.boost.status
	@echo "[$@] done"

simula-boost-status: boost_status
boost_status: $(SIMULATION_DEPS_SOURCE)/.boost.status
	@echo "[$@] `cat $(SIMULATION_DEPS_SOURCE)/.boost.status`"


simula-vsomeip: vsomeip
vsomeip: boost
	$(call setup-simulation-dep,$@)

simula-vsomeip-download: vsomeip_download
vsomeip_download:
	@echo "[$@] downloading from [$(VSOMEIP_URL)]" && wget -q -O \
		$(SIMULATION_DEPS_SOURCE)/$(_VSOMEIP_VERSION) $(VSOMEIP_URL) > /dev/null
	@echo "[$@] extract to [$(SIMULATION_DEPS_SOURCE)/vsomeip]" && cd $(SIMULATION_DEPS_SOURCE) \
		&& if [ -d vsomeip ]; then rm -r vsomeip; fi \
		&& tar xfz $(_VSOMEIP_VERSION) && mv $(VSOMEIP_VERSION) vsomeip
	@echo -n "Inited" > $(SIMULATION_DEPS_SOURCE)/.vsomeip.status
	@echo "[$@] to Inited"

simula-vsomeip-build: vsomeip_build
vsomeip_build: $(SIMULATION_DEPS_INSTALL)/boost/include $(SIMULATION_DEPS_SOURCE)/vsomeip
	@if ! [ -e $(SIMULATION_DEPS_SOURCE)/.vsomeip.status ]; then \
		echo "[$@] You MUST download source-code, try: vsomeip_download as target" ; exit 1 ; fi
	@if ! [ -d $(SIMULATION_DEPS_INSTALL)/vsomeip ]; then \
		mkdir -p $(SIMULATION_DEPS_INSTALL)/vsomeip ; fi
	@cd $(SIMULATION_DEPS_SOURCE)/vsomeip \
		&& echo "[$@] configure vsomeip firstly" \
			&& if [ -d build ]; then rm -rf build ; fi \
				&& mkdir -p build && cd build \
				&& cmake -DBoost_INCLUDE_DIR=$(SIMULATION_DEPS_INSTALL)/boost/include \
					-DBoost_LIBRARY_DIR=$(SIMULATION_DEPS_INSTALL)/boost/lib \
					-DENABLE_SIGNAL_HANDLING=1 \
					-DCMAKE_INSTALL_PREFIX=$(SIMULATION_DEPS_INSTALL)/vsomeip .. > ./__config.log 2>&1 \
		&& echo "[$@] compiling vsomeip ..." \
			&& make > ./__build.log 2>&1
	@echo -n "Compiled" > $(SIMULATION_DEPS_SOURCE)/.vsomeip.status
	@echo "[$@] to Compiled"

simula-vsomeip-install: vsomeip_install
vsomeip_install: $(SIMULATION_DEPS_INSTALL)/boost/include $(SIMULATION_DEPS_SOURCE)/vsomeip/build
	@if ! [ -e $(SIMULATION_DEPS_SOURCE)/.vsomeip.status ] || [ "`cat $(SIMULATION_DEPS_SOURCE)/.vsomeip.status`" = "Inited" ]; then \
		echo "[$@] You MUST build source-code, try: vsomeip_build as target" ; exit 1 ; fi
	@cd $(SIMULATION_DEPS_SOURCE)/vsomeip/build \
		&& if [ -d $(SIMULATION_DEPS_INSTALL)/vsomeip ]; then \
			echo "[$@] remove old install" && rm -r $(SIMULATION_DEPS_INSTALL)/vsomeip; fi \
		&& mkdir -p $(SIMULATION_DEPS_INSTALL)/vsomeip \
		&& echo "[$@] installing ..." \
		&& make install > ./__install.log 2>&1
	@echo -n "Installed" > $(SIMULATION_DEPS_SOURCE)/.vsomeip.status
	@echo "[$@] to Installed"

simula-vsomeip-uninstall: vsomeip_uninstall
vsomeip_uninstall: $(SIMULATION_DEPS_INSTALL)/boost/include $(SIMULATION_DEPS_SOURCE)/vsomeip/build
	@echo "[$@] un-installing"
	@rm -rf $(SIMULATION_DEPS_INSTALL)/vsomeip
	@echo -n "Compiled" > $(SIMULATION_DEPS_SOURCE)/.vsomeip.status
	@echo "[$@] to Compiled"

simula-vsomeip-clean: vsomeip_clean
vsomeip_clean: $(SIMULATION_DEPS_INSTALL)/boost/include $(SIMULATION_DEPS_SOURCE)/vsomeip/build
	@cd $(SIMULATION_DEPS_SOURCE)/vsomeip/build \
		&& echo "[$@] cleaning ..." \
		&& make clean > /dev/null \
		&& cd ../ && rm -rf build
	@echo -n "Inited" > $(SIMULATION_DEPS_SOURCE)/.vsomeip.status
	@echo "[$@] to Inited"

simula-vsomeip-remove vsomeip_remove: vsomeip_remove_install vsomeip_remove_source

simula-vsomeip-remove-install: vsomeip_remove_install
vsomeip_remove_install:
	@echo "[$@] removing install ..."
	@rm -rf $(SIMULATION_DEPS_INSTALL)/vsomeip
	@echo "[$@] done"

simula-vsomeip-remove-source: vsomeip_remove_source
vsomeip_remove_source:
	@echo "[$@] removing source & .vsomeip.status file ..."
	@rm -rf $(SIMULATION_DEPS_SOURCE)/vsomeip
	@if [ -e $(SIMULATION_DEPS_SOURCE)/.vsomeip.status ]; then \
		rm -f $(SIMULATION_DEPS_SOURCE)/.vsomeip.status ; fi
	@echo "[$@] done"

simula-vsomeip-status: vsomeip_status
vsomeip_status: $(SIMULATION_DEPS_SOURCE)/.vsomeip.status
	@echo "[$@] `cat $(SIMULATION_DEPS_SOURCE)/.vsomeip.status`"


which_one_default := $(CURDIR)/simulation/which_one_default
which_one := $(CURDIR)/simulation/workstation/.which_one
get_which_one := `if [ -e $(which_one) ]; then cat $(which_one) ; else cat $(which_one_default) ; fi`
which_one_point_version :=  $(shell echo $(get_which_one) | sed 's/\([0-9][0-9]\)/\1./')

# If you want to specify a private hub address to get ubuntu base images, override 'from' in commands
export from ?=

ifneq ("$(origin from)","command line")
export from := ubuntu:$(which_one_point_version)
endif

define up_simulation_container
	@echo "Up Simulation with [$(1:up_%.sh=%)]"
	@/bin/bash $(CURDIR)/simulation/scripts/$(1) $(get_which_one) $(2)
	@echo "Down Simulation with [$(1:up_%.sh=%)], see you ~"
endef

define build_simulation_docker_image
	@echo "[$@] build docker image..."
	@export UBUNTU_DISTRO_ORIGIN=$(from) \
	    && docker compose -f "$(CURDIR)/simulation/docker/for_ubuntu_$(get_which_one)/docker-compose.yml" \
	    build telaf_simulation_$(1)_$(get_which_one)
	@echo "[$@] image build done."
endef

simula-st simula-subsys-tests: $(LEGATO_ROOT)/build/simulation
	$(MAKE) -C $(LEGATO_ROOT) subsys_tests TARGET=simulation TELAF_ROOT_SET=$(TELAF_ROOT)

simula-tests_c:
	$(MAKE) -C $(LEGATO_ROOT) tests_c TARGET=simulation TELAF_ROOT_SET=$(TELAF_ROOT)
	@ln -sf $(LEGATO_RELATIVE_PATH)/build ./build

simula-help:
	@echo "Help Page for TelAF Simulation CLI"
	@echo
	@echo "> make simula-action [simula-action ...] [simula-action-args]"
	@echo
	@echo "  Examples:"
	@echo "    > make simula-list"
	@echo "    > make simula within='make simula'"
	@echo
	@echo "  >> simula-action-args"
	@echo "    - within='command'"
	@echo "    - from='hub-address'"
	@echo
	@echo "  >> simula-action supported list as follows"
	@echo "    - List & Switch simulation container distro system versions (default Ubuntu18.04)"
	@echo "      + simula-list                    -- List all system distro versions simulation supported."
	@echo "      + simula-distro-1804             -- Switch the system distro version to Ubuntu18.04"
	@echo "      + simula-distro-2004             -- Switch the system distro version to Ubuntu20.04"
	@echo
	@echo "    - Compile your simulation project on your HOST or CONTAINER"
	@echo "      + simula | simulac               -- Incrementally compile simulation open source code on HOST"
	@echo "      + simula-clean                   -- Just deep clean your simulation project"
	@echo "      + simula within='<command>'      -- Incrementally compile simulation open source code in CONTAINER"
	@echo
	@echo "    - Build your simulation docker containers cli, depends which system version you selected (see 'simula-list')"
	@echo "      + simula-build-runtime           -- Build a runtime docker image for running TelAF Simulation"
	@echo "      + simula-build-develop           -- Build a develop docker image for developing Simulation in it"
	@echo "      + simula-build-all               -- Build all docker images along with [runtime, develop, oncecmd]"
	@echo "      + simula-build-runtime from='hub-address'"
	@echo "                                       -- Specify a hub address you want to get the ubuntu base image and build it"
	@echo
	@echo "    - Boot up your simulation docker container that was built, depends which system version you selected (see 'simula-list')"
	@echo "      + simula-up | simula-up-runtime  -- Boot up the runtime container to simulate"
	@echo "      + simula-upx| simula-upx-runtime -- Boot up multi-runtime-containers to simulate"
	@echo "      + simula-up-develop              -- Boot up the develop container for developers"
	@echo
	@echo "    - Docker operation helper commands"
	@echo "      + simula-listimg                 -- List all docker images on your host"
	@echo "      + simula-listv                   -- List all volumes named along with 'telaf'"
	@echo "      + simula-rmv                     -- Delete all volumes named along with 'telaf'"


simula-buildall simula-build-all-docker-images: simula-build-runtime simula-build-develop

simula-build simula-build-runtime:
	$(call build_simulation_docker_image,runtime)

simula-build-develop:
	$(call build_simulation_docker_image,develop)

simula-up simula-up-runtime:
	$(call up_simulation_container,up_runtime_master.sh)

# slave-x containers are daemons, start first.
simula-upx simula-upx-runtime:
	$(call up_simulation_container,up_runtime_slavex.sh)
	$(call up_simulation_container,up_runtime_master.sh)

simula-up-develop:
	$(call up_simulation_container,up_develop.sh)

simula-up-develop-for-c:
	$(call up_simulation_container,up_develop.sh,$(within))

simula-list simula-list-distro:
	@echo "TelAF Simulation support list:"
	@echo -n "  [1] ubuntu20.04"; if [ "$(get_which_one)" = "2004" ]; then echo " <--" ; else echo ; fi
	@echo -n "  [2] ubuntu18.04"; if [ "$(get_which_one)" = "1804" ]; then echo " <--" ; else echo ; fi

simula-distro-1804:
	@echo -n "1804" > $(which_one)
	@echo "Switch container OS version to --> 18.04"
	@echo
	@$(MAKE) --no-print-directory simula-list

simula-distro-2004:
	@echo -n "2004" > $(which_one)
	@echo "Switch container OS version to --> 20.04"
	@echo
	@$(MAKE) --no-print-directory simula-list

simula-listimg simula-list-all-docker-images:
	@echo "[$@] list all simulation images"
	@docker images

simula-rm-dangling simula-remove-docker-dangling-images:
	@echo "[$@] Removing dangling images..."
	@docker rmi $$(docker images -f "dangling=true" -q)
	@echo "[$@] remove dangling images done."

simula-listv simula-list-simulation-volumes:
	@echo "[$@] docker just for simulation volumes as the list"
	@docker volume ls -qf "name=telaf"

simula-listallv simula-list-all-volumes:
	@echo "[$@] docker all volumes as the list"
	@docker volume ls

simula-rmv simula-remove-simulation-volumes:
	@echo "[$@] detele all telaf simulation volumes ..."
	@docker volume rm $$(docker volume ls -qf "name=telaf")
	@echo "[$@] detele all telaf simulation volumes done."

simula-remove-all-volumes:
	@echo "[$@] detele all volumes ..."
	@volumes=$$(docker volume ls -q) && { for volume in $$volumes ; do docker volume rm $$volume ; done }
	@echo "[$@] detele all volumes done."

simula-clean: distclean
