//******************************************************************************
// NAME
// OCS_IPN_EventReporter.cpp
//
// COPYRIGHT Ericsson AB, Sweden 2012.
// All rights reserved.
//
// The Copyright to the computer program(s) herein
// is the property of Ericsson AB, Sweden.
// The program(s) may be used and/or copied only with
// the written permission from Ericsson AB or in
// accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been
// supplied.

// DOCUMENT NO
// 190 89-CAA 109 1405

// AUTHOR
// 2012-02-23 by XDT/DEK XTUANGU

// DESCRIPTION
// This class handles events to be reported to AEH.

// LATEST MODIFICATION
// -
//******************************************************************************

#include "OCS_IPN_EventReporter.h"
#include "OCS_IPN_Trace.h"

#include <sstream>
#include <iomanip>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

OCS_IPN_EventReporter* OCS_IPN_EventReporter::singletonPtr=0;
unsigned int OCS_IPN_EventReporter::noOfUsers=0;
//*************************************************************************
// getInstance (static)
//*************************************************************************
OCS_IPN_EventReporter* OCS_IPN_EventReporter::getInstance(void)
{
	if (singletonPtr==0)
	{
		singletonPtr=new OCS_IPN_EventReporter();
	}
	if (singletonPtr) noOfUsers++;

	return singletonPtr;
}

//******************************************************************************
// deleteInstance() 
//******************************************************************************
void OCS_IPN_EventReporter::deleteInstance(OCS_IPN_EventReporter* tptr)
{
	if (tptr==singletonPtr)
	{
		noOfUsers--;
		if (singletonPtr && noOfUsers==0)
		{
			delete singletonPtr;
			singletonPtr=0;
		}
	}
}

//*************************************************************************
// constructor
//*************************************************************************
OCS_IPN_EventReporter::OCS_IPN_EventReporter()
: events(100,INITEVENT),
  evp(new acs_aeh_evreport())
{

	stringstream ss;
	ss<<"ocs_ipnaadmd:"<<getpid();
	procName=ss.str();
	

	// 36000-36099 for IPNAADM

	//36000 - Software error from OI/OM
	events[0].probableCause="IMM";
	events[0].objectOfRef="SOFTWARE";

}

//*************************************************************************
// Destructor
//*************************************************************************
OCS_IPN_EventReporter::~OCS_IPN_EventReporter()
{
	if (singletonPtr && evp)
	{
		singletonPtr=0;
		delete evp;
	}
}

//*************************************************************************
// reportEvent
//
// Report event to AEH.
//
// return value:
//   -
//*************************************************************************
void OCS_IPN_EventReporter::reportEvent(int problemNo,string probData,string objRef)
{
        newTRACE((LOG_LEVEL_INFO, "OCS_IPN_EventReporter::reportEvent",0));

        // Get objectofRef from pre-defined if not provided.
        if (objRef.compare("")==0)
        {
           objRef=events[problemNo-10000].objectOfRef;
        }

        ACS_AEH_ReturnType err = ACS_AEH_ok;

        // check return code of this method!!!!!
        err = evp->sendEventMessage(procName.c_str(),
                       problemNo,
                       "EVENT",
                       events[problemNo-10000].probableCause.c_str(),
                       "APZ",
                       objRef.c_str(),
                       probData.c_str(),
                       events[problemNo-10000].problemText.c_str());

        // Send event to TRACE
        ostringstream trace;
        if (ACS_AEH_error == err)
        {
            ACS_AEH_ErrorType errCode = evp->getError();
            const char* errorText = evp->getErrorText();
            trace << endl << "OCS_IPN_EventReporter::reportEvent failed" <<endl;
            trace << "ACS_AEH_EvReport::sendEventMessage failed" << endl;
            trace << "Error Code = " << errCode << endl;
            trace << "Error Test = \"" << errorText << "\"" << endl;
        }
        else
        {
            trace << endl << "OCS_IPN_EventReporter::reportEvent" << endl;
        }

        trace << "EVENT = " << problemNo << endl;
        trace << events[problemNo-10000].probableCause.c_str() << endl;
        trace << objRef.c_str() << endl;
        trace << probData.c_str() << endl;

        TRACE((LOG_LEVEL_INFO, trace.str().c_str(),0));
}
