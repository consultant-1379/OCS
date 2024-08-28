# **********************************************************************
#
# Short description:
# Makefile template for ipnaadm_adm_caa
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
# Rev        Date	  Name      What
# -----      ---------    --------  --------------------------
#			2010-07-13   xtuangu   Created
#			2011-08-22   xtuangu	Update following the Makefile DR PB2
#           2016-01-26   egiaava   Adapted for SLES12 and new BOOST LIB 1_54
#
# ********************************************************************
CNZ_NAME ?= ipnaadm_cnz
CAA_NAME ?= ipnaadm_adm_caa
CXC_NAME ?= ipnaadm_cxc

# Basic paths
VOB_PATH = $(OCS_ROOT)
CNZ_PATH = $(VOB_PATH)/$(CNZ_NAME)
CAA_PATH = $(CNZ_PATH)/$(CAA_NAME)
CXC_PATH = $(CNZ_PATH)/$(CXC_NAME)
                                  
include $(COMMON_ROOT)/common.mk
                                 
DOXYGENFILECONFIG := $(COMMON_ROOT)/doxygen_common.conf

# define objs want to build
OBJS = 	$(OBJDIR)/OCS_IPN_Main.o \
		$(OBJDIR)/OCS_IPN_Thread.o \
		$(OBJDIR)/OCS_IPN_Server.o \
		$(OBJDIR)/OCS_IPN_Service.o \
		$(OBJDIR)/OCS_IPN_Utils.o \
		$(OBJDIR)/OCS_IPN_Common.o \
		$(OBJDIR)/OCS_IPN_Trace.o \
		$(OBJDIR)/OCS_IPN_EventReporter.o \
		$(OBJDIR)/OCS_IPN_CpRelatedSwManagerOI.o
		

CINCLUDES += -I$(INCDIR)

## # here you can add own Include paths and/or other INCDIRludes
CINCLUDES += -I$(AP_SDK_INC) -I$(BOOST_SDK_INC) -I$(3PP_SDK_PATH) -I$(COREMW_SDK_INC) -I$(ACE_SDK_INC)

## # here you can add your own lib dirs
LIBSDIR += -L$(AP_SDK_LIB) -L$(BOOST_SDK_LIB) -L$(ACE_SDK_LIB) -L$(COREMW_SDK_LIB)

## # here you can add own libs 
## TODO: remove this line & unmark the above line when need to use IMM & ACE stuff 
LIBS += -lacs_apgcc -lacs_csapi -lacs_jtp -lacs_dsd -lacs_tra -lboost_thread\
-lpthread -lboost_system -lSaImmOm -lacs_aeh -lacs_prc

CFLAGS += -O3 -g -Wall -ansi

LDFLAGS += -Wl,-rpath-link,$(AP_SDK_LIB)
LDFLAGS += -Wl,-rpath-link,$(COREMW_SDK_LIB)
LDFLAGS += -Wl,-rpath-link,$(SEC_SDK_LIB)
LDFLAGS += -Wl,-rpath-link,$(SEC_CREDU_SDK_LIB)
LDFLAGS += -Wl,-rpath-link,$(LIB_XERCES_SDK_LIB)
LDFLAGS += -Wl,-rpath-link,$(BOOST_SDK_LIB)
LDFLAGS += -Wl,-rpath-link,$(ACE_SDK_LIB)

VPATH += $(SRCDIR) $(INCDIR) $(OUTDIR) $(OBJDIR)

IPNAADM_SERVER_APNAME = ocs_ipnaadmd
IPNAADM_SERVER_APEXE = $(OUTDIR)/$(IPNAADM_SERVER_APNAME)

.PHONY: all
all: $(IPNAADM_SERVER_APNAME)

.PHONY: $(IPNAADM_SERVER_APNAME)  
$(IPNAADM_SERVER_APNAME): $(OBJS)
	$(SILENT)$(ECHO) 'Linking $@...'
	$(CC) -o $(IPNAADM_SERVER_APEXE) $(OBJS) $(LDFLAGS) $(LIBSDIR) $(LIBS)
	$(call stripp,$(IPNAADM_SERVER_APNAME))
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
	$(SILENT)$(RM) -r $(OBJDIR)/*
	
.PHONY: distclean
distclean: clean
	$(SILENT)$(RM) -r $(OUTDIR)/$(IPNAADM_SERVER_APNAME)
	$(SILENT)$(RM)  -r $(DEBUGDIR)/$(IPNAADM_SERVER_APNAME)_dbg


