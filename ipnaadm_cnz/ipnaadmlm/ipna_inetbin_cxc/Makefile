# **********************************************************************
#
# Short description:
# Makefile template for XXX library
# **********************************************************************
#
# Ericsson AB 2010 All rights reserved.
# The information in this document is the property of Ericsson.
# Except as specifically authorized in writing by Ericsson, the receiver of this
# document shall keep the information contained herein confidential and shall protect
# the same in whole or in part from disclosure and dissemination to third parties.
# Disclosure and disseminations to the receivers employees shall only be made
# on a strict need to know basis.
#
# **********************************************************************
#
# Rev        Date         Name      What
# -----      ----------   --------  --------------------------
#            2010-11-18   xtuangu   Created
#            2011-02-09   xchihoa   Update following Makefile_DR_PA6 (Lb: SDK_APG43L_PROJ_0504_002)
#            2011-06-27   xchihoa   Update following Makefile_DR_PA8 (Lb: SDK_APG43L_PROJ_1101_002)
#                                                            
# **********************************************************************
 
# BEGIN: GIT INTRO.
CURDIR = $(shell pwd)
REPO_NAME = ocs
OCS_ROOT = $(shell echo $(CURDIR) | sed 's@'/$(REPO_NAME)'.*@'/$(REPO_NAME)'@g')
COMMON_ROOT = $(OCS_ROOT)/common

include $(COMMON_ROOT)/common.mk

CXC_NAME?=IPNA_INET
CXC_NR?=CXC1060087
CXC_VER?=R1G01
CXC_OLD_VER?=R1G01

CURDIR = $(shell pwd)
CXCPATH=$(CURDIR)
CXCDIR=$(CXCPATH)
export CXCDIR

DOXYGEN_OUTPUT=$(DOCDIR)/docgen
BLOCK_NAME=$(CXC_NAME)
BLOCK_VERSION=$(CXC_NAME)_$(CXC_VER)
BLOCK_SEARCH=../../ipnaadm_adm_caa/src ../../ipnaadm_adm_caa/inc
export DOXYGEN_OUTPUT
export BLOCK_NAME
export BLOCK_VERSION
export BLOCK_SEARCH

.PHONY: all clean distclean documentation metrics cppcheck_m cccc_m sloccount_m
all clean distclean documentation metrics cppcheck_m cccc_m sloccount_m: 
	$(SILENT)$(ECHO) "Do nothing"

.PHONY: nightbuild
nightbuild: prebuild distclean all rpm_package sdp_package undobuild

.PHONY: release
release: all rpm_package sdp_package tar_package ap_sdk documentation metrics

.PHONY: rpm_package
rpm_package:
	$(call rpm_file_creation,ipna_inet.spec,$(CXC_NAME),$(CXC_NR),$(CXC_VER))

.PHONY: sdp_package
sdp_package:
	$(call sdp_creation_US1,$(CXCPATH),$(BLOCK_NAME),$(CXC_VER),$(CXC_OLD_VER),$(CXC_NR)) 

.PHONY: tar_package
tar_package:
	$(call tar_creation,$(BLOCK_NAME),$(CXC_NR),$(CXC_VER))

.PHONY: help
help:
	$(SILENT)$(ECHO) 'Usage: make [target] [option]'
	$(SILENT)$(ECHO) 'target:'
	$(SILENT)$(ECHO) '  all                Build all targets (default)'
	$(SILENT)$(ECHO) '  release            Build all targets and create rpm, metrics'
	$(SILENT)$(ECHO) '  clean              Removes object files'
	$(SILENT)$(ECHO) '  distclean          Removes object and binary files'	
	$(SILENT)$(ECHO) '  help               Print help'
	$(SILENT)$(ECHO) 'option:'
	$(SILENT)$(ECHO) '  BUILD_VERBOSE=0|1  (0: quiet build (default), 1: verbose build)'
	$(SILENT)$(ECHO) '  COVERAGE=0|1       (0: no coverage (default), 1: coverage)'
	

