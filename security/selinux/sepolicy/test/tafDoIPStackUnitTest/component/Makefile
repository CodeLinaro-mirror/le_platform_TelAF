# Makefile for building SELinux module

AWK ?= gawk
NAME ?= $(strip $(shell $(AWK) -F= '/^SELINUXTYPE/{ print $$2 }' $(OECORE_TARGET_SYSROOT)/etc/selinux/config))
SHAREDIR := ${TMPDIR}/work-shared/selinux/mls

ifeq ($(MLSENABLED),)
	MLSENABLED := 1
endif

ifeq ($(MLSENABLED),1)
	NTYPE = mcs
endif

ifeq ($(NAME),mls)
	NTYPE = mls
endif

TYPE ?= $(NTYPE)

HEADERDIR := $(SHAREDIR)/include
include $(HEADERDIR)/Makefile
