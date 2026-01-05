
top_of_project=$(CURDIR)/../../../../
project_name=$(shell basename $(shell realpath ../../../../))
ifneq ($(project_name),telaf)
  $(error Please check if you are in the telaf project first !)
endif

diagcfg_of_project=$(shell realpath $(top_of_project)/components/tafDiagSvc/tafDiagCfg)
SERIALIZATION_SRC := $(diagcfg_of_project)/serialization.cpp
SERIALIZATION_HOST_TARGET := $(diagcfg_of_project)/serialization.host

BUILD_CXX      ?= g++
BUILD_CXXFLAGS ?= -O2
BUILD_LDFLAGS  ?=

STAGING_INCDIR_NATIVE ?=
STAGING_LIBDIR_NATIVE ?=

HOST_CPPFLAGS += $(if $(STAGING_INCDIR_NATIVE),-I$(STAGING_INCDIR_NATIVE),)

HOST_LDFLAGS := $(if $(STAGING_LIBDIR_NATIVE),-L$(STAGING_LIBDIR_NATIVE),) -lboost_serialization


ifeq ($(DGTOOL_TARGET),)
  PY=python3
else
  $(info target is $(DGTOOL_TARGET))
  PY=$(top_of_project)/apps/tools/tafDiagGen/venv/$(DGTOOL_TARGET)/bin/python3
endif

.PHONY: default install help clean version serialize

default:
	$(PY) $(CURDIR)/tafDiagGen

version help:
	$(PY) $(CURDIR)/tafDiagGen $@

setup setup-py-environment:
	$(PY) --version
	which $(PY) | xargs -r ls -l
	$(PY) -m pip list
	@echo "[Setup] Install python3 required packages..."
	@$(PY) -m pip install -r $(CURDIR)/requirements.txt
	@echo "[Setup] Installed."
	$(PY) -m pip list

install: do-install
do-install:
	cp -af $(CURDIR)/build/configuration.cpp $(diagcfg_of_project)/
	cp -af $(CURDIR)/build/configuration.hpp $(diagcfg_of_project)/
	cp -af $(CURDIR)/build/diag_template.yaml.json $(diagcfg_of_project)/
	cp -af $(CURDIR)/build/diag_ids.h $(diagcfg_of_project)/
	cp -af $(CURDIR)/serialization/serialization.cpp $(diagcfg_of_project)/
	cp -af $(CURDIR)/serialization/serialization.hpp $(diagcfg_of_project)/
	@echo "[Install] Done."

serialize: do-serialize
do-serialize: $(SERIALIZATION_HOST_TARGET)
	@LD_LIBRARY_PATH="$(STAGING_LIBDIR_NATIVE):$$LD_LIBRARY_PATH" \
	$(SERIALIZATION_HOST_TARGET) $(diagcfg_of_project)/
	@echo "[Serialize] Done."

$(SERIALIZATION_HOST_TARGET): $(SERIALIZATION_SRC)
	mkdir -p $(dir $@)
	$(BUILD_CXX) $(BUILD_CXXFLAGS) -o $@ $^ $(BUILD_LDFLAGS) $(HOST_CPPFLAGS) $(HOST_LDFLAGS)

clean:
	@rm -rf build
	@rm -rf $(SERIALIZATION_HOST_TARGET)
