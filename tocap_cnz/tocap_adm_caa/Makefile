# **********************************************************************
#
# Short description:
# Makefile template for tocap_adm_caa
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
#	     2010-12-10   xtuangu   Created
#        2011-02-09   xchihoa   Update following Makefile_DR_PA5 (Lb: SDK_APG43L_PROJ_0504_002)
#        2011-06-28   xchihoa   Update following Makefile_DR_PA8 (Lb: SDK_APG43L_PROJ_1101_002)
#		 2011-10-19   xtuangu	Update follow the Makefile DR PB2
#        2016-01-21   egiaava   Adapted for SLES12 and new BOOST LIB 1_54
#
# ********************************************************************
CNZ_NAME ?= tocap_cnz
CAA_NAME ?= tocap_adm_caa
CXC_NAME ?= tocap_cxc

# Basic paths
VOB_PATH = $(OCS_ROOT)
CNZ_PATH = $(VOB_PATH)/$(CNZ_NAME)
CAA_PATH = $(CNZ_PATH)/$(CAA_NAME)
CXC_PATH = $(CNZ_PATH)/$(CXC_NAME)
                                  
include $(COMMON_ROOT)/common.mk
                                 
DOXYGENFILECONFIG := $(COMMON_ROOT)/doxygen_common.conf

# define objs want to build
OBJS = 	$(OBJDIR)/OCS_OCP_Service.o \
		$(OBJDIR)/OCS_OCP_svcMain.o \
		$(OBJDIR)/OCS_OCP_CSfunctions.o \
		$(OBJDIR)/OCS_OCP_events.o \
		$(OBJDIR)/SocketException.o \
		$(OBJDIR)/Socket.o \
		$(OBJDIR)/UDPSocket.o \
		$(OBJDIR)/OCS_OCP_protHandler.o \
		$(OBJDIR)/OCS_OCP_session.o \
		$(OBJDIR)/OCS_OCP_sessionFE.o \
		$(OBJDIR)/OCS_OCP_sessionFEO.o \
		$(OBJDIR)/OCS_OCP_sessionNFE.o \
		$(OBJDIR)/OCS_OCP_link.o \
		$(OBJDIR)/OCS_OCP_alarmFile.o \
		$(OBJDIR)/OCS_OCP_alarmMgr.o \
		$(OBJDIR)/OCS_OCP_frontEndAP.o \
		$(OBJDIR)/OCS_OCP_nonFrontEndAP.o \
		$(OBJDIR)/OCS_OCP_Server.o \
		$(OBJDIR)/OCS_OCP_Trace.o \
		$(OBJDIR)/OCS_OCP_CheckAPNodeState.o \
		$(OBJDIR)/OCS_OCP_HWCTableObserver.o
			

CINCLUDES += -I$(INCDIR)

## # here you can add own Include paths and/or other INCDIRludes
CINCLUDES += -I$(AP_SDK_INC) -I$(BOOST_SDK_INC) -I$(3PP_SDK_PATH) -I$(COREMW_SDK_INC) -I$(LOTC_SDK_INC)\
-I$(ACE_SDK_INC)

## # here you can add your own lib dirs
LIBSDIR += -L$(AP_SDK_LIB) -L$(BOOST_SDK_LIB) -L$(LIB_LOGCPLUS_SDK_LIB) -L$(LOTC_SDK_LIB)

## # here you can add own libs 
## TODO: remove this line & unmark the above line when need to use IMM & ACE stuff  
LIBS += -lacs_apgcc -lacs_csapi -lacs_dsd -lacs_tra -lacs_prc -lacs_aeh -llog4cplus\
-lpthread -lboost_thread -lboost_system

CFLAGS += -O3 -g -Wall -ansi

LDFLAGS += -Wl,-rpath-link,$(ACE_SDK_LIB)
LDFLAGS += -Wl,-rpath-link,$(COREMW_SDK_LIB)
LDFLAGS += -Wl,-rpath-link,$(SEC_SDK_LIB)
LDFLAGS += -Wl,-rpath-link,$(SEC_CREDU_SDK_LIB)
LDFLAGS += -Wl,-rpath-link,$(BOOST_SDK_LIB)

VPATH += $(SRCDIR) $(INCDIR) $(OUTDIR) $(OBJDIR)

TOCAP_APNAME = ocs_tocapd
TOCAP_APEXE = $(OUTDIR)/$(TOCAP_APNAME)

.PHONY: all
all: $(TOCAP_APNAME)

.PHONY: $(TOCAP_APNAME)  
$(TOCAP_APNAME): $(OBJS)
	$(SILENT)$(ECHO) 'Linking $@...'
	$(CC) -o $(TOCAP_APEXE) $(OBJS) $(LDFLAGS) $(LIBSDIR) $(LIBS)
	$(call stripp,$(TOCAP_APNAME))
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
	$(SILENT)$(RM) -r $(OUTDIR)/$(TOCAP_APNAME)
	$(SILENT)$(RM) -r $(DEBUGDIR)/$(TOCAP_APNAME)_dbg

	
