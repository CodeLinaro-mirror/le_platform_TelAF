
top_of_project=$(CURDIR)/../../../../
project_name=$(shell basename $(shell realpath ../../../../))
ifneq ($(project_name),telaf)
  $(error Please check if you are in the telaf project first !)
endif

diagcfg_of_project=$(shell realpath $(top_of_project)/components/tafDiagSvc/tafDiagCfg)

PY=python3

.PHONY: default install help clean version

default:
	$(PY) $(CURDIR)/tafDiagGen

version help:
	$(PY) $(CURDIR)/tafDiagGen $@

setup setup-py-environment:
	@echo "[Setup] Install python3 required packages..."
	@$(PY) -m pip install -r $(CURDIR)/requirements.txt
	@echo "[Setup] Installed."

install: do-install
do-install:
	cp -af $(CURDIR)/build/configuration.cpp $(diagcfg_of_project)/
	cp -af $(CURDIR)/build/configuration.hpp $(diagcfg_of_project)/
	cp -af $(CURDIR)/build/diag_template.yaml.json $(diagcfg_of_project)/
	cp -af $(CURDIR)/build/diag_ids.h $(diagcfg_of_project)/
	@echo "[Install] Done."

clean:
	@rm -rf build
