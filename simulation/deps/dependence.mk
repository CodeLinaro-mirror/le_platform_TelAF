
# Here we handle all the host tool dependencies, as well as the dependencies inside the container

# Stores the source code and zip for all dependent projects
export SIMULATION_DEPS_SOURCE := $(SIMULATION_HOME)/deps/source

# Dependencies to install in docker containers
export SIMULATION_DEPS_ROOTFS := $(SIMULATION_HOME)/deps/taf_rootfs

# Required extend tools to be installed in dev HOST
export SIMULATION_HOST_XTOOLS := $(SIMULATION_HOME)/deps/host_xtools

# This file with a valid format of xx.xx or empty are used to check compatibility
deps_origin=$(SIMULATION_HOME)/deps/.deps.origin

precheck_deps := \
  if [ -e "$(deps_origin)" ]; then \
    ORIG_VERSION=$$(cat $(deps_origin)) ; \
    if echo "$$ORIG_VERSION" | grep -qE '^[0-9][0-9]\.[0-9][0-9]$$'; then \
      if [ "$(OS_VERSION)" = "$$ORIG_VERSION" ]; then \
        echo "Prechecking dependencies is consistent [OK]" ; \
      else \
        echo ; \
        echo "Prechecking dependencies is [NOK]" ; \
        echo "Origin from: $$ORIG_VERSION" ; \
        echo "Current ver: $(OS_VERSION)" ; \
        echo ; \
        echo "Ignore this warning ..." ; \
		echo "If you want to be consistent, delete [$(deps_origin)]" and rebuild all; \
        echo ; \
      fi ; \
    else \
      echo "Invalid context in [$(deps_origin)], continue ... " ; \
    fi ; \
  else \
    echo "Not found file [$(deps_origin)]" ; \
	echo "Touch it and clean all 3rd party deps, rebuilding all dpes ... " ; \
    touch $(deps_origin) && rm -rf $(SIMULATION_DEPS_ROOTFS)/* $(SIMULATION_HOST_XTOOLS)/* ; \
  fi

mark_deps_origin := \
  if ! [ -s "$(deps_origin)" ]; then \
    echo -n "$(OS_VERSION)" > $(deps_origin) ; \
    echo "Mark 3rd party dependencies from [$(OS_VERSION)]" ; \
  fi

# All pre actions for handling dependencies.
# Check the existed 3rd party dependencies if fulfill current system
_pre_deps:
	$Q $(precheck_deps)

# All post actions for handling dependencies.
# Mark the 3rd party dependencies that are from which ubuntu distro version
# to be used by the precondition checking steps
_post_deps:
	$Q $(mark_deps_origin)

# -- The steps for all dependencies --
#-> [clean]    1. Clean original compression files and directories (every time)
#-> [download] 2. Download & extract the compression files
#-> [compile]  3. Compile the dependent project
#-> [install]  4. Install all generated stuff to SIMULATION_DEPS_ROOTFS or SIMULATION_HOST_XTOOLS

.PHONY: boost

BOOST_URL?=https://boostorg.jfrog.io/artifactory/main/release/1.74.0/source/boost_1_74_0.tar.gz
_BOOST_VERSION=$(notdir $(lastword $(subst /, ,$(BOOST_URL))))
BOOST_VERSION=$(_BOOST_VERSION:%.tar.gz=%)

# $(error $(shell ls -l $(SIMULATION_DEPS_ROOTFS)/include/boost/version.hpp))

_boost: $(SIMULATION_DEPS_ROOTFS)/include/boost/version.hpp
	$Q echo "[$@] Already preparation"

$(SIMULATION_DEPS_ROOTFS)/include/boost/version.hpp:
	@$(MAKE) --no-print-directory simula-boost

simula-boost: boost_
boost_:
#-> 1. [clean]
	$Q echo "[$@] cleaning compression and directories" \
	  && rm -rf $(SIMULATION_DEPS_SOURCE)/$(_BOOST_VERSION) $(SIMULATION_DEPS_SOURCE)/$@
#-> 2. [download]
	$Q echo "[$@] downloading from [$(BOOST_URL)]" \
	  && wget -q -O $(SIMULATION_DEPS_SOURCE)/$(_BOOST_VERSION) $(BOOST_URL) > /dev/null
	$Q echo "[$@] extract to [$(SIMULATION_DEPS_SOURCE)/$@]" \
	  && cd $(SIMULATION_DEPS_SOURCE) \
	  && tar xfz $(_BOOST_VERSION) && mv $(BOOST_VERSION) $@
#-> 3. [compile]
	$Q cd $(SIMULATION_DEPS_SOURCE)/$@ \
	  && echo "[$@] configure firstly" \
	  && ./bootstrap.sh --prefix=$(SIMULATION_DEPS_ROOTFS) > ./__config.log 2>&1 \
	  && echo "[$@] compiling ..."  \
	  && ./b2 > ./__build.log 2>&1
#-> 4. [install]
	$Q cd $(SIMULATION_DEPS_SOURCE)/$@ \
	  && echo "[$@] installing ..." \
	  && ./b2 install > ./__install.log 2>&1
	$Q echo "[$@] Done"


.PHONE: cmake

CMAKE_URL?=https://cmake.org/files/v3.15/cmake-3.15.3.tar.gz
_CMAKE_VERSION=$(notdir $(lastword $(subst /, ,$(CMAKE_URL))))
CMAKE_VERSION=$(_CMAKE_VERSION:%.tar.gz=%)

# $(error $(shell ls -l $(SIMULATION_HOST_XTOOLS)/bin/cmake))

_cmake: $(SIMULATION_HOST_XTOOLS)/bin/cmake
	$Q echo "[$@] Already preparation"

$(SIMULATION_HOST_XTOOLS)/bin/cmake:
	@$(MAKE) --no-print-directory simula-cmake

simula-cmake: cmake_
cmake_:
#-> 1. [clean]
	$Q echo "[$@] cleaning compression and directories" \
	  && rm -rf $(SIMULATION_DEPS_SOURCE)/$(_CMAKE_VERSION) $(SIMULATION_DEPS_SOURCE)/$@
#-> 2. [download]
	$Q echo "[$@] downloading from [$(BOOST_URL)]" \
	  && wget -q -O $(SIMULATION_DEPS_SOURCE)/$(_CMAKE_VERSION) $(CMAKE_URL) > /dev/null
	$Q echo "[$@] extract to [$(SIMULATION_DEPS_SOURCE)/$@]" \
	  && cd $(SIMULATION_DEPS_SOURCE) \
	  && tar xfz $(_CMAKE_VERSION) && mv $(CMAKE_VERSION) $@
#-> 3. [compile]
	$Q cd $(SIMULATION_DEPS_SOURCE)/$@ \
	  && echo "[$@] configure firstly" \
	  && ./configure --prefix=$(SIMULATION_HOST_XTOOLS) > ./__config.log 2>&1 \
	  && echo "[$@] compiling ..."  \
	  && make -j $(shell nproc) > ./__build.log 2>&1
#-> 4. [install]
	$Q cd $(SIMULATION_DEPS_SOURCE)/$@ \
	  && echo "[$@] installing ..." \
	  && make install > ./__install.log 2>&1
	$Q echo "[$@] Done"


.PHONE: vsomeip

# VSOMEIP_URL?=https://github.com/COVESA/vsomeip/archive/refs/tags/3.1.20.3.tar.gz
# NOTE: cmake >= 3.13.xx is required for vsomeip 3.4.9-r1
VSOMEIP_URL?=https://github.com/COVESA/vsomeip/archive/refs/tags/3.4.9-r1.tar.gz
_VSOMEIP_VERSION=$(notdir $(lastword $(subst /, ,$(VSOMEIP_URL))))
VSOMEIP_VERSION=$(_VSOMEIP_VERSION:%.tar.gz=vsomeip-%)

_vsomeip: $(SIMULATION_DEPS_ROOTFS)/include/vsomeip/vsomeip.hpp
	$Q echo "[$@] Already preparation"

$(SIMULATION_DEPS_ROOTFS)/include/vsomeip/vsomeip.hpp:
	$Q $(MAKE) --no-print-directory simula-vsomeip

simula-vsomeip: vsomeip_
vsomeip_: _cmake _boost
#-> 1. [clean]
	$Q echo "[$@] cleaning compression and directories" \
	  && rm -rf $(SIMULATION_DEPS_SOURCE)/$(_VSOMEIP_VERSION) $(SIMULATION_DEPS_SOURCE)/$@
#-> 2. [download]
	$Q echo "[$@] downloading from [$(VSOMEIP_URL)]" \
	  && wget -q -O $(SIMULATION_DEPS_SOURCE)/$(_VSOMEIP_VERSION) $(VSOMEIP_URL) > /dev/null
	$Q echo "[$@] extract to [$(SIMULATION_DEPS_SOURCE)/$@]" \
	  && cd $(SIMULATION_DEPS_SOURCE) \
	  && tar xfz $(_VSOMEIP_VERSION) && mv $(VSOMEIP_VERSION) $@
#-> 3. [compile]
	$Q cd $(SIMULATION_DEPS_SOURCE)/$@ \
	  && echo "[$@] configure firstly" \
	    && mkdir -p build && cd build \
	    && $(SIMULATION_HOST_XTOOLS)/bin/cmake \
	      -DBoost_INCLUDE_DIR=$(SIMULATION_DEPS_ROOTFS)/include \
	      -DBoost_LIBRARY_DIR=$(SIMULATION_DEPS_ROOTFS)/lib \
	      -DENABLE_SIGNAL_HANDLING=1 \
	      -DCMAKE_INSTALL_PREFIX=$(SIMULATION_DEPS_ROOTFS) .. > ./__config.log 2>&1 \
	  && echo "[$@] compiling ..." \
	    && make > ./__build.log 2>&1
#-> 4. [install]
	$Q cd $(SIMULATION_DEPS_SOURCE)/$@/build \
	  && echo "[$@] installing ..." \
	    && make install > ./__install.log 2>&1
	$Q echo "[$@] Done"


.PHONE: openssl

OPENSSL_URL?=https://github.com/openssl/openssl.git
OPENSSL_VERSION=openssl-3.0.9

_openssl: $(SIMULATION_DEPS_ROOTFS)/include/openssl/opensslconf.h
	$Q echo "[$@] Already preparation"

$(SIMULATION_DEPS_ROOTFS)/include/openssl/opensslconf.h:
	$Q $(MAKE) --no-print-directory simula-openssl

simula-openssl: openssl_
openssl_:
#-> 1. [clean]
	$Q echo "[$@] cleaning compression and directories" \
	  && rm -rf $(SIMULATION_DEPS_SOURCE)/$@
#-> 2. [download]
	$Q echo "[$@] downloading from [$(OPENSSL_URL)]" \
	  && git clone -q --depth 1 --branch ${OPENSSL_VERSION} --single-branch \
	         ${OPENSSL_URL} $(SIMULATION_DEPS_SOURCE)/$@ > ./__download.log 2>&1
	$Q echo "[$@] just from git repo, no need to extract"
#-> 3. [compile]
	$Q cd $(SIMULATION_DEPS_SOURCE)/$@ \
	  && echo "[$@] configure firstly" \
	    && ./Configure --prefix=${SIMULATION_DEPS_ROOTFS} > ./__config.log 2>&1 \
	  && echo "[$@] compiling ..." \
	    && make > ./__build.log 2>&1
#-> 4. [install]
	$Q cd $(SIMULATION_DEPS_SOURCE)/$@ \
	  && echo "[$@] installing ..." \
	    && make install_sw > ./__install.log 2>&1
	$Q echo "[$@] Done"


.PHONE: curl

CURL_URL?=https://github.com/curl/curl.git
CURL_VERSION=curl-7_69_1

_curl: $(SIMULATION_DEPS_ROOTFS)/include/curl/curl.h
	$Q echo "[$@] Already preparation"

$(SIMULATION_DEPS_ROOTFS)/include/curl/curl.h:
	$Q $(MAKE) --no-print-directory simula-curl

simula-curl: curl_
curl_:
#-> 1. [clean]
	$Q echo "[$@] cleaning compression and directories" \
	  && rm -rf $(SIMULATION_DEPS_SOURCE)/$@
#-> 2. [download]
	$Q echo "[$@] downloading from [$(CURL_URL)]" \
	  && git clone -q --depth 1 --branch ${CURL_VERSION} --single-branch \
	         ${CURL_URL} $(SIMULATION_DEPS_SOURCE)/$@ > ./__download.log 2>&1
	$Q echo "[$@] just from git repo, no need to extract"
#-> 3. [compile]
	$Q cd $(SIMULATION_DEPS_SOURCE)/$@ \
	  && echo "[$@] configure firstly" \
	    && mkdir -p build && cd build \
	    && cmake -DCMAKE_INSTALL_PREFIX=${SIMULATION_DEPS_ROOTFS} .. > ./__config.log 2>&1 \
	  && echo "[$@] compiling ..." \
	    && make > ./__build.log 2>&1
#-> 4. [install]
	$Q cd $(SIMULATION_DEPS_SOURCE)/$@/build \
	  && echo "[$@] installing ..." \
	    && make install > ./__install.log 2>&1
	$Q echo "[$@] Done"
