
top_of_project=$(CURDIR)/../../../..

diagcfg_of_project=$(shell realpath $(top_of_project)/components/tafDiagSvc/tafDiagCfg)

ifeq ($(DGTOOL_TARGET),)
  PY=python3
else
  $(info target is $(DGTOOL_TARGET))
  PY=$(top_of_project)/apps/tools/tafDiagGen/venv/$(DGTOOL_TARGET)/bin/python3
endif

.PHONY: full default install

default: full

DEF_LOG_LVL ?= INFO

ifneq ($(DEBUG),)
export DE_PREFIX_MSG=
DEF_LOG_LVL := DEBUG
endif

PYTHON_OK := $(shell $(PY) -c 'import sys; print(1) if sys.version_info >= (3, 8) else print(0)')

full full-feature: python-version-check
	$(PY) ./tafDiagGen --log-level $(DEF_LOG_LVL) \
                       -s $(CURDIR)/samples/schemas \
                       -c $(CURDIR)/samples/customer \
                       -e $(CURDIR)/samples/extended

python-version-check:
	@ if [ $(PYTHON_OK) -ne 1 ]; then \
	    echo "Python version requires (>= 3.8.0)"; \
	    echo "If on Ubuntu, please try:" ; \
	    echo "    sudo apt update" ; \
	    echo "    sudo apt install software-properties-common" ; \
	    echo "    sudo add-apt-repository ppa:deadsnakes/ppa" ; \
	    echo "    sudo apt update" ; \
	    echo "    sudo apt install python3.8" ; \
	    echo "    sudo update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.8 20" ; \
	    echo "    sudo update-alternatives --set python3 /usr/bin/python3.8" ; \
	    echo "    sudo update-alternatives --list python3 # (check)" ; \
	    echo "    sudo update-alternatives --config python3 # (switch)" ; \
	    exit 1; \
	fi

setup setup-py-environment: python-version-check
	$(PY) --version
	which $(PY) | xargs -r ls -l
	$(PY) -m pip list
	@echo "[Setup] Install python3 required packages..."
	@$(PY) -m pip install -r $(CURDIR)/requirements.txt
	@echo "[Setup] Installed."
	$(PY) -m pip list

install: do-install
do-install:
	@echo "[Install] configuration ..full.. feature installed"
	@cp -f build/* $(diagcfg_of_project)/

clean:
	@rm -rf build backtrace.log record.log

test:
	@$(PY) $(CURDIR)/run_tests.py

show-test show-test-example:
	@echo "Note: make sure the Python-Import-Path including your project."
	@echo "Please change your CURDIR to the project first <-- cd /path/to/project"
	@echo
	@echo "unittest CLI Syntax:"
	@echo "  python -m unittest discover -s tests"
	@echo "  python -m unittest test_module1 test_module2"
	@echo "  python -m unittest test_module.TestCase"
	@echo "  python -m unittest test_module.TestCase.test_method"
	@echo "unittest CLI examples:"
	@echo "  python -m unittest tests.test_schema_map.Test_SchemaSyntax_map.test_map_with_bad_parameters_include"
	@echo "  python -m unittest tests.test_schema_map.Test_SchemaSyntax_map."
	@echo "  python -m unittest tests.test_schema_map"
