# --------------------------------------------------------------------------------------------------
# Makefile for TelAF Simulation Target
# --------------------------------------------------------------------------------------------------

TARGETS += simulation menuconfig_simulation

# For outputing the simulation related details, within 'Q=' in make command
# BTW, for all details for TelAF & Legato, within 'Q= V=1' in make command
Q ?=@

-include $(TELAF_ROOT)/simulation/.simulation.build

# If you want make a compilation in your docker container, get along with below.
export within ?=

# Sometimes, due to docker's caching, it can lead docker image rebuilding failure.
# So we need to add some options for building images, such as "--no-cache"
docker_build_opts ?=

# Some work needs to be done earlier or later, so we prevent the real simulation goal.
ifneq ($(filter simulation,$(MAKECMDGOALS)),)
  $(error Please pass 'make simula-help' for TelAF Simulation Target [simulation])
endif

# Override the LEGATO_VERSION, to ensure the version is same as other targets.
export LEGATO_VERSION=$(shell cat $(TELAF_ROOT)/VERSION 2> /dev/null)

# For embedded target, the cross-compilation tool will change the 'sysroot'
# to search header & libraries that have beed relocated.
# when we get the path by '--print-sysroot', that value will be returned.
# But for simulation target, the default prefix (empty) is used,
# because the host default environment for the gcc compiler is that.
# So for consistency, we add the required path without any affect for mktools.
export SYSROOT=/

export SIMULATION_HOME := $(TELAF_ROOT)/simulation
export SIMULATION_SCRIPTS := $(SIMULATION_HOME)/scripts
export SIMULATION_WORKDIR := $(SIMULATION_HOME)/workstation
SIMULATION_TARBALL := $(SIMULATION_HOME)/workstation/telaf_simulation.tar

# Sub-Makefile to handle all target dependencies and extended host tools
include $(SIMULATION_HOME)/deps/dependence.mk

SIMULATION_DEPS += # Empty is default, but it is post-extended

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
  # $(warning sdk rootfs path [$(sdk_rootfs)])
endif

MKTOOLS_FLAGS_SIMULATION_EX += --cxxflags=-I$(sdk_rootfs)/include \
                               --cxxflags=-I$(SIMULATION_DEPS_ROOTFS)/include \
                               --ldflags=-L$(sdk_rootfs)/lib \
                               --ldflags=-L$(SIMULATION_DEPS_ROOTFS)/lib

export TELAF_SIMULATION_ENABLE_SMS ?= n
export TELAF_SIMULATION_ENABLE_DCS ?= n
export TELAF_SIMULATION_ENABLE_SIM ?= n
export TELAF_SIMULATION_ENABLE_LOC ?= n
export TELAF_SIMULATION_ENABLE_RADIO ?= n

export TELAF_SIMULATION_ENABLE_MNGD_CONN ? = n
ifneq ($(TELAF_SIMULATION_ENABLE_MNGD_CONN),n)
  TELAF_SIMULATION_ENABLE_DCS := y
  TELAF_SIMULATION_ENABLE_SIM := y
  TELAF_SIMULATION_ENABLE_RADIO := y
endif

endif

export TELAF_SIMULATION_ENABLE_SOMEIP_GW ?= y
export TELAF_SIMULATION_ENABLE_DIAG ?= n

SIMULATION_SOMEIP_GW_DEPS_y := _vsomeip
SIMULATION_DEPS += $(SIMULATION_SOMEIP_GW_DEPS_$(TELAF_SIMULATION_ENABLE_SOMEIP_GW))

MKTOOLS_FLAGS_SIMULATION_EX += -X -std=c++11 -X -lstdc++

ifneq ($(TELAF_SIMULATION_ENABLE_SOMEIP_GW),n)
  MKTOOLS_FLAGS_SIMULATION_EX += \
    --cxxflags=-I$(SIMULATION_DEPS_ROOTFS)/include \
	--ldflags=-L$(SIMULATION_DEPS_ROOTFS)/lib
endif

export MKTOOLS_FLAGS_SIMULATION_EX

.PHONY: simulation

ifeq ($(within),)

  ifeq ($(DEBUG),on)
    export DEBUG=1 STRIP_STAGING_TREE=0
    simula simulac simula-c: simula-clean-config check-sys pre-simulation-build simulation post-simulation-build
  else
    simula simulac simula-c: check-sys pre-simulation-build simulation post-simulation-build
  endif # end DEBUG

else # below includes the appending 'within' option

  ifeq ($(DEBUG),on)
    export DEBUG=1 STRIP_STAGING_TREE=0
    simula simulac simula-c: simula-clean-config simula-up-develop-for-c
  else
    simula simulac simula-c: simula-up-develop-for-c
  endif # end DEBUG

endif # end within

OS_VERSION=$(shell grep -oP 'VERSION_ID=\K"(.+)"' /etc/os-release | tr -d '"')

check-sys:
	$Q echo "TelAF Simulation pre-checking your system ..."
	$Q if [ -e $(SIMULATION_WORKDIR)/.check_done ] && [ "`umask`" = "0022" ]; then \
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


pre-simulation-build: $(SIMULATION_HOME)/workstation/up_simulation.sh $(SIMULATION_DEPS)
post-simulation-build: CURRENT_SYSTEM_OUTPUT=$(TELAF_BUILD)/simulation/_staging_system.simulation.update_ro/systems/current
post-simulation-build:
	$Q echo "[Simulation]: Creating Tarball ..."
	$Q tar cf $(SIMULATION_TARBALL) -C $(TELAF_BUILD)/simulation/_staging_system.simulation.update_ro .
	$Q tar rf $(SIMULATION_TARBALL) -C $(SIMULATION_HOME)/workstation/ up_simulation.sh
	$Q tar rf $(SIMULATION_TARBALL) -C $(SIMULATION_HOME)/workstation/ .check_done
ifneq ($(TELAF_SIMULATION_ENABLE_MNGD_CONN),n)
	$Q cp $(TELAF_ROOT)/apps/sample/TelAF-CM/JSON/mngdConnectivity.json $(SIMULATION_HOME)/deps/taf_rootfs
endif
	$Q tar rf $(SIMULATION_TARBALL) --exclude=taf_rootfs/include \
	                                --exclude=taf_rootfs/lib/cmake \
	                                --exclude=taf_rootfs/lib/pkgconfig \
	                                --exclude=taf_rootfs/etc \
	                                -C $(SIMULATION_HOME)/deps taf_rootfs
ifneq ($(CHECK_SDK_ROOTFS),n)
	$Q tar rf $(SIMULATION_TARBALL) --transform 's/rootfs/sdk_rootfs/' -C $(sdk_rootfs)/../ rootfs
endif
ifneq ($(TELAF_SIMULATION_ENABLE_DIAG),n)
	$Q cp $(TELAF_ROOT)/apps/tools/diag/diag_test_38_36_37.py $(SIMULATION_WORKDIR)/
else
	$Q if [ -e "$(SIMULATION_WORKDIR)/diag_test_38_36_37.py" ]; then rm -f $(SIMULATION_WORKDIR)/diag_test_38_36_37.py ; fi
endif
	$Q gzip -f $(SIMULATION_TARBALL)
	$Q echo "[Simulation]: Tarball $(SIMULATION_TARBALL).gz done."


simula-menuconfig: menuconfig_simulation

which_one_default := $(CURDIR)/simulation/which_one_default
which_one := $(CURDIR)/simulation/workstation/.which_one
get_which_one := `if [ -e $(which_one) ]; then cat $(which_one) ; else cat $(which_one_default) ; fi`
which_one_point_version :=  $(shell echo $(get_which_one) | sed 's/\([0-9][0-9]\)/\1./')

# If you want to specify a private hub address to get ubuntu base images, override 'from' in commands
export from ?=

ifneq ("$(origin from)","command line")
  export from := ubuntu:$(which_one_point_version)
else
  # Check if the current version matches the version passed in, if not, please 'make simula-distro-<version>'
  FROM_TYPICAL_VERSION=$(shell echo $(from) | sed 's/.*:\(.*\)/\1/')
  ifneq ("$(FROM_TYPICAL_VERSION)","$(which_one_point_version)")
    $(error "Mismatched version, current wanted [$(which_one_point_version)], but got [$(FROM_TYPICAL_VERSION)].\
	         Please check 'make simula-list' to ensure the version to be supported.")
  endif

  # NOTE: in addition to the officially supported versions, you can create your own, but we will not give technical support
  # Step 1. Create directory & Dockerfiles: <version> -> 2204, <dot-version> -> 22.04
  #      1.1 simulation/docker/for_ubuntu_<version>
  #      1.2 simulation/docker/for_ubuntu_<version>/docker-compose.yml
  #      1.2.2 Add the docker-compose.yml with correct Dockerfile names
  #      1.3 simulation/docker/for_ubuntu_<version>/Dockerfile.<version>.runtime
  #      1.4 simulation/docker/for_ubuntu_<version>/Dockerfile.<version>.develop
  # Step 2. Change the 'simulation/workstation/.which_one' to which <version> you want
  # Step 3. Rebuild all images: make simula-buildall from=ubuntu:<dot-version>

endif

define up_simulation_container
	$Q echo "Up Simulation with [$(1:up_%.sh=%)]"
	$Q /bin/bash $(CURDIR)/simulation/scripts/$(1) $(get_which_one) $(2)
	$Q echo "Down Simulation with [$(1:up_%.sh=%)], see you ~"
endef

define build_simulation_docker_image
	$Q echo "[$@] build docker image..."
	$Q export UBUNTU_DISTRO_ORIGIN=$(from) \
	    && docker compose -f "$(CURDIR)/simulation/docker/for_ubuntu_$(get_which_one)/docker-compose.yml" \
	    build $(docker_build_opts) telaf_simulation_$(1)_$(get_which_one)
	$Q echo "[$@] image build done."
endef

simula-st simula-subsys-tests: $(LEGATO_ROOT)/build/simulation
	$Q $(MAKE) -C $(LEGATO_ROOT) subsys_tests TARGET=simulation TELAF_ROOT_SET=$(TELAF_ROOT)

simula-tests_c:
	$Q $(MAKE) -C $(LEGATO_ROOT) tests_c TARGET=simulation TELAF_ROOT_SET=$(TELAF_ROOT)
	$Q ln -sf $(LEGATO_RELATIVE_PATH)/build ./build

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
	$Q echo "TelAF Simulation support list:"
	$Q echo -n "  [1] ubuntu20.04"; if [ "$(get_which_one)" = "2004" ]; then echo " <--" ; else echo ; fi
	$Q echo -n "  [2] ubuntu18.04"; if [ "$(get_which_one)" = "1804" ]; then echo " <--" ; else echo ; fi
	$Q if [ "$(get_which_one)" != "1804" ] \
	   && [ "$(get_which_one)" != "2004" ]; then \
	      echo -n "  [x] ubuntu$(which_one_point_version)"; echo " <-- (unknown version)" ; fi

simula-distro-1804:
	$Q echo -n "1804" > $(which_one)
	$Q echo "Switch container OS version to --> 18.04"
	$Q echo
	$Q $(MAKE) --no-print-directory simula-list

simula-distro-2004:
	$Q echo -n "2004" > $(which_one)
	$Q echo "Switch container OS version to --> 20.04"
	$Q echo
	$Q $(MAKE) --no-print-directory simula-list

simula-listimg simula-list-all-docker-images:
	$Q echo "[$@] list all simulation images"
	$Q docker images

simula-rm-dangling simula-remove-docker-dangling-images:
	$Q echo "[$@] Removing dangling images..."
	$Q docker rmi $$(docker images -f "dangling=true" -q)
	$Q echo "[$@] remove dangling images done."

simula-listv simula-list-simulation-volumes:
	$Q echo "[$@] docker just for simulation volumes as the list"
	$Q docker volume ls -qf "name=telaf"

simula-listallv simula-list-all-volumes:
	$Q echo "[$@] docker all volumes as the list"
	$Q docker volume ls

simula-rmv simula-remove-simulation-volumes:
	$Q echo "[$@] detele all telaf simulation volumes ..."
	$Q docker volume rm $$(docker volume ls -qf "name=telaf")
	$Q echo "[$@] detele all telaf simulation volumes done."

simula-remove-all-volumes:
	$Q echo "[$@] detele all volumes ..."
	$Q volumes=$$(docker volume ls -q) && { for volume in $$volumes ; do docker volume rm $$volume ; done }
	$Q echo "[$@] detele all volumes done."

simula-clean: distclean

simula-clean-config:
	$Q rm -f $(LEGATO_ROOT)/.config.simulation

simula-clean-system:
	$Q rm -rf build/simulation/{_staging_system.simulation.update,system}

simula-rm-network:
	$Q docker network rm $$(docker network ls -q --filter="name=telaf_simulation_runtime") > /dev/null

simula-build-config simula-bc:
	$Q cat $(TELAF_ROOT)/simulation/.simulation.build
