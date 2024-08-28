# **********************************************************************
#
# Short description:
# Makefile template for ipnaadm_cmd_caa
# **********************************************************************
#
# Ericsson AB 2011 All rights reserved.
# The information in this document is the property of Ericsson.
# Except as specifically authorized in writing by Ericsson, the receiver of this
# document shall keep the information contained herein confidential and shall protect
# the same in whole or in part from disclosure and dissemination to third parties.
# Disclosure and disseminations to the receivers employees shall only be made
# on a strict need to know basis.
#
# **********************************************************************
#
# Rev		Date     Name      What
# -----      ---------    --------  --------------------------
#			2011-07-21   xngudan   Created
#			2011-08-22   xtuangu	Update following the Makefile DR PB2
#           2016-01-26   egiaava   Adapted for SLES12 and new BOOST LIB 1_54
# ********************************************************************
CURDIR = $(shell pwd)
REPO_NAME = ocs
OCS_ROOT = $(shell echo $(CURDIR) | sed 's@'/$(REPO_NAME)'.*@'/$(REPO_NAME)'@g')
COMMON_ROOT = $(OCS_ROOT)/common
 
include $(COMMON_ROOT)/common.mk

IPNAADMSERVER_DIR = ../../ipnaadm_adm_caa
CAA_OBJS_DIR = $(IPNAADMSERVER_DIR)/obj
CAA_SRC_DIR = $(IPNAADMSERVER_DIR)/src
# define objs want to build
OBJS = 	$(OBJDIR)/OCS_IPN_Cmd.o \
		$(OBJDIR)/OCS_IPN_ExtFnx.o 
 
CAA_OBJS = $(CAA_OBJS_DIR)/OCS_IPN_Utils.o \
		   $(CAA_OBJS_DIR)/OCS_IPN_Trace.o

CINCLUDES += -I$(INCDIR)

## # here you can add own Include paths and/or other INCDIRludes
CAA_DIRS_INC = ../../ipnaadm_adm_caa/inc
CINCLUDES += -I$(AP_SDK_INC) -I$(BOOST_SDK_INC) -I$(CAA_DIRS_INC) -I$(3PP_SDK_PATH)\
-I$(COREMW_SDK_INC) -I$(ACE_SDK_INC)

## # here you can add your own lib dirs
LIBSDIR += -L$(AP_SDK_LIB) -L$(BOOST_SDK_LIB) -L$(COREMW_SDK_LIB)
## # here you can add own libs 
## TODO: remove this line & unmark the above line when need to use IMM & ACE stuff 
LIBS += -lacs_apgcc -lacs_tra -lpthread -lSaImmOm -lacs_dsd -lacs_csapi -lacs_prc\
-lboost_system

## # TODO: remove this when FMS is available

CFLAGS += -O3 -g -Wall -ansi

LDFLAGS += -Wl,-rpath-link,$(AP_SDK_LIB)
LDFLAGS += -Wl,-rpath-link,$(COREMW_SDK_LIB)
LDFLAGS += -Wl,-rpath-link,$(SEC_SDK_LIB)
LDFLAGS += -Wl,-rpath-link,$(SEC_CREDU_SDK_LIB)
LDFLAGS += -Wl,-rpath-link,$(ACE_SDK_LIB)
LDFLAGS += -Wl,-rpath-link,$(BOOST_SDK_LIB)

VPATH += $(SRCDIR) $(INCDIR) $(OUTDIR) $(OBJDIR)

IPNA_APNAME = ipnaadm
IPNA_APEXE = $(OUTDIR)/$(IPNA_APNAME)

.PHONY: all
all: $(IPNA_APNAME)

.PHONY: $(IPNA_APNAME)  
$(IPNA_APNAME): $(OBJS) $(CAA_OBJS)
	$(SILENT)$(ECHO) 'Linking $@...'
	$(SILENT)$(CC) -o $(IPNA_APEXE) $(OBJS) $(LDFLAGS) $(CAA_OBJS) $(LIBSDIR) $(LIBS)
	$(call stripp,$(IPNA_APNAME))
	$(NEW_LINE)	
  
$(CAA_OBJS_DIR)/%.o: $(CAA_SRC_DIR)/%.cpp
	$(SILENT)$(ECHO) 'Generating dependencies for $<...'
	@$(call make-depend,$<,$@,$(subst .o,.d,$@))
	$(SILENT)$(ECHO) "Compiling $<..."
	$(SILENT)$(CC) $(GCOV_FLAGS) -c $(CFLAGS) $(CINCLUDES) $< -o $@
	$(NEW_LINE)

# Include dependecies in order to build when header file changes
DEPS = $(OBJS:.o=.d)
ifeq ($(MAKECMDGOALS),all)	
	-include $(DEPS)
else
	ifeq ($(MAKECMDGOALS),)
		-include $(DEPS)
	endif
endif

.PHONY: clean
clean:
	$(SILENT)$(RM)  $(OBJDIR)/*
	$(SILENT)$(RM)  $(CAA_OBJS_DIR)/*
	
.PHONY: distclean
distclean: clean
	$(SILENT)$(RM)  -r $(OUTDIR)/$(IPNA_APNAME)
	$(SILENT)$(RM)  -r $(DEBUGDIR)/$(IPNA_APNAME)_dbg

	
