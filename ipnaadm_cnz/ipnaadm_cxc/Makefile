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
#            2011-08-22   xtuangu   Created
#			 2011-08-22   xtuangu	Update following the Makefile DR PB2
#			 2016-02-02   egiaava	Updated CXC
#                                                            
# **********************************************************************
# BEGIN: GIT INTRO.
CURDIR = $(shell pwd)
REPO_NAME = ocs
OCS_ROOT = $(shell echo $(CURDIR) | sed 's@'/$(REPO_NAME)'.*@'/$(REPO_NAME)'@g')
COMMON_ROOT = $(OCS_ROOT)/common

include $(COMMON_ROOT)/common.mk 

CXC_NAME?=OCS_IPNAADMBIN
CXC_NR?=CXC1372129_9
CXC_VER?=R1C
#CXC_OLD_VER?=R1C
BASE_SW_VER?=3.6.0-R1A
CXC_BLK_NAME ?= ocs_ipnaadm
CURDIR = $(shell pwd)
CXCPATH=$(CURDIR)
CXCDIR=$(CXCPATH)
export CXCDIR

FX_CLEAN_TMP := rm -rf /tmp/$(USER)/*

DOXYGEN_OUTPUT=$(DOCDIR)/docgen
BLOCK_NAME=$(CXC_NAME)
BLOCK_VERSION=$(CXC_NAME)_$(CXC_VER)
BLOCK_SEARCH=../ipnaadm_adm_caa/src ../ipnaadm_adm_caa/inc
export DOXYGEN_OUTPUT
export BLOCK_NAME
export BLOCK_VERSION
export BLOCK_SEARCH

CAA_DIRS = \
		../ipnaadm_adm_caa \
		../ipnaadm_cmd_caa
		
# Define cppcheck source/inc		
CPPCKECK_SRCS = ../ipnaadm_adm_caa/src/*.cpp ../ipnaadm_cmd_caa/src/*.cpp
CPPCKECK_INCS = -I../ipnaadm_adm_caa/inc -I../ipnaadm_cmd_caa/inc

# Deinfe cccc source
CCCC_SRCS = ../ipnaadm_adm_caa/src/* ../ipnaadm_cmd_caa/src/*

# Define SLOCCOUNT src
SLOCCOUNT_SRCS = ../ipnaadm_adm_caa/ ../ipnaadm_cmd_caa/

.PHONY: all clean distclean
all clean distclean: 
	@for i in $(CAA_DIRS) ; do \
		(echo "cd $$i" ; \
		 cd $$i ; \
		 $(MAKE) $(MFLAGS) COMMON_ROOT=$(COMMON_ROOT) OCS_ROOT=$(OCS_ROOT) $@) ;\
	done

.PHONY: nightbuild
nightbuild: del_update distclean all rpm_package sdp_package esm_package 

.PHONY: del_update
del_update:
	$(call rpm_del_update,$(CXCPATH),$(BLOCK_NAME),$(CXC_VER),$(OLD_CXC_VER),$(CXC_NR))
	$(call sdp_del_update,$(CXCPATH),$(BLOCK_NAME),$(CXC_VER),$(OLD_CXC_VER),$(CXC_NR))
	$(call esm_del_update,$(CXCPATH),$(BLOCK_NAME),$(CXC_VER),$(OLD_ESM_VER),$(CXC_NR))

.PHONY: release
release: all rpm_package sdp_package esm_package tar_package ap_sdk documentation metrics

.PHONY: rpm_package
rpm_package:
	$(call rpm_file_creation,ipnaadm.spec,$(CXC_NAME),$(CXC_NR),$(CXC_VER))

.PHONY: sdp_package
sdp_package:
	$(call sdp_creation_US3,$(CXCPATH),$(BLOCK_NAME),$(CXC_VER),$(CXC_OLD_VER),$(CXC_NR))
	$(call up_creation,$(CXC_NAME),$(CXC_NR),$(CXC_VER),$(CXC_OLD_VER),$(CXCPATH),$(BASE_SW_VER))

.PHONY: esm_package
esm_package:
	$(call esm_pkg_creation,$(CXCPATH),$(CXC_BLK_NAME),$(CXC_VER),$(BASE_SW_VER),$(CXC_NR),$(CXC_NAME))
                      
.PHONY: upload_package 
upload_package: 
	$(call upload_package,$(CXC_BLK_NAME),$(CXC_NR),$(CXC_NAME),$(CXC_VER))

 

.PHONY: tar_package
tar_package:
	$(call tar_creation,$(BLOCK_NAME),$(CXC_NR),$(CXC_VER))

.PHONY: metrics
metrics: cppcheck_m cccc_m sloccount_m

.PHONY: documentation
documentation:
	$(call doc_creation)

.PHONY: cppcheck_m
cppcheck_m:		
	$(call cppcheck_doc_creation,$(CPPCKECK_SRCS),$(CPPCKECK_INCS))

.PHONY: cccc_m
cccc_m:		
	$(call cccc_doc_creation,$(CCCC_SRCS))

.PHONY: sloccount_m
sloccount_m:
	$(call sloccount_doc_creation,$(SLOCCOUNT_SRCS))
	
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
	

