# **********************************************************************
#
# Short description:
# Makefile template for echo_client
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
# Rev		 Date		  Name      What
# -----      ---------    --------  --------------------------
#		     2010-12-10   xtuangu   Created
#
# ********************************************************************

# the following variables are defined in common.mk file lines 369
#SRCDIR = src
#OBJDIR = obj
#OUTDIR = $(CXCDIR)/bin
#INCDIR = inc
OTHEROBJDIR = ../../tocap_adm_caa/obj
OTHERINCDIR = ../../tocap_adm_caa/inc
CXCDIR = ../../tocap_cxc

CURDIR = $(shell pwd)
REPO_NAME = ocs
OCS_ROOT = $(shell echo $(CURDIR) | sed 's@'/$(REPO_NAME)'.*@'/$(REPO_NAME)'@g')
COMMON_ROOT = $(OCS_ROOT)/common
 
include $(COMMON_ROOT)/common.mk

# define objs want to build
OBJS =  $(OBJDIR)/TCP_Client.o \
				$(OBJDIR)/TCP_Connection.o \
				$(OBJDIR)/DSD_Client.o \
				$(OBJDIR)/Menu.o \
				$(OBJDIR)/t_main.o \
				$(OTHEROBJDIR)/OCS_OCP_events.o \
            	$(OTHEROBJDIR)/SocketException.o \
            	$(OTHEROBJDIR)/Socket.o \
            	$(OTHEROBJDIR)/UDPSocket.o \
            	$(OTHEROBJDIR)/OCS_OCP_protHandler.o
            	
				      
# define AP-SDK dirs
AP_SDK_INCDIR = $(AP_SDK_INC)
AP_SDK_LIBDIR = $(AP_SDK_LIB)

# define boost dirs
BOOST_INCDIR = $(BOOST_SDK_INC)
BOOST_LIBDIR = $(BOOST_SDK_LIB)

MIDDLEWARE_LIBDIR2 = $(COREMW_SDK_LIB)/opensaf

#LOG4CPLUS_LIB =../../../../cm4ap/ntacs/tra_cnz/tra_cxc/bin

CINCLUDES += -I$(INCDIR) -I$(OTHERINCDIR)
## # here you can add own Include paths and/or other INCDIRludes
CINCLUDES += -I$(AP_SDK_INC) -I$(BOOST_INCDIR)

## # here you can add your own lib dirs
LIBSDIR += -L$(AP_SDK_LIB) -L$(BOOST_SDK_LIB) -L$(LIB_LOGCPLUS_SDK_LIB)

## # here you can add own libs 
##LIBS += -lboost_regex -lboost_system -lboost_thread
## TODO: remove this line & unmark the above line when need to use IMM & ACE stuff 
LIBS += -lacs_dsd -lacs_tra -lacs_aeh -llog4cplus -lboost_thread -lboost_system -lboost_date_time

#LIBS += -lACS_CSAPI -lACS_DSD -lboost_thread -lboost_system -lopensaf_core -lboost_date_time
##LIBS +=  -lboost_thread -lboost_system -lboost_date_time

## # TODO: remove this when FMS is available
EXTRA_CFLAGS += -DLOCAL_BUILD 

CFLAGS += -O3 -g -Wall -ansi

TARGET= t_tocap

.PHONY: all
all: $(TARGET)

.PHONY: $(TARGET)  
$(TARGET): $(OBJS)
	$(SILENT)$(ECHO) 'Linking $@...'
	$(CC) -o $(OUTDIR)/$(TARGET) $(OBJS) $(LIBSDIR) $(LIBS)
	$(NEW_LINE)	

# this rule is defined in common.mk
#$(OBJDIR)/%.o: $(SRCDIR)/%.cpp

.PHONY: clean
clean:
	$(RM) -r $(OBJDIR)/*.o
	$(RM) -r $(OBJDIR)/*.d
	$(RM) $(OUTDIR)/$(TARGET)

