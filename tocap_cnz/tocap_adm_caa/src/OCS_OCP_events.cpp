//******************************************************************************
// NAME
// OCS_OCP_events.cpp
//
// COPYRIGHT Ericsson AB, Sweden 2007.
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
// 190 89-CAA 109 0748

// AUTHOR 
// 2007-11-29 by EAB/FTE/DDH UABCAJN

// DESCRIPTION
// This class is a singleton class, i.e only one instance exists.
// This class handles events and alarms to be reported to AEH.

// LATEST MODIFICATION
// -
//******************************************************************************

#include "OCS_OCP_events.h"
#include "OCS_OCP_Trace.h"
//#include "acs_aeh_types.h"

#include <sstream>
#include <iomanip>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;


TOCAP_Events* TOCAP_Events::singletonPtr=0;
unsigned int TOCAP_Events::noOfUsers=0;
//*************************************************************************
// getInstance (static)
//*************************************************************************
TOCAP_Events* TOCAP_Events::getInstance(void)
{
    if (singletonPtr==0)
    {
        singletonPtr=new TOCAP_Events();
    }
    if (singletonPtr) noOfUsers++;

    return singletonPtr;
}

//******************************************************************************
// deleteInstance() 
//******************************************************************************
void TOCAP_Events::deleteInstance(TOCAP_Events* tptr)
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
TOCAP_Events::TOCAP_Events()
: events(100,INITEVENT),
  eventCnt(100,0),
  eventTime(100,0),
  evp(new acs_aeh_evreport())
{

    stringstream ss;
    ss<<"ocs_tocapd:"<<getpid();
    procName=ss.str();


    // 10000
    events[0].probableCause="ERROR RETRIEVING DATA FROM CS";
    events[0].objectOfRef="FE/CS";

    // 10001
    events[1].probableCause="MISSING CLUSTER DATA";
    events[1].objectOfRef="FE/CS";

    // 10002
    events[2].probableCause="OPEN DSD SERVER FAILURE";
    events[2].objectOfRef="FE/DSD";

    // 10003
    events[3].probableCause="GET SERVER HANDLES FAILURE";
    events[3].objectOfRef="FE/DSD";

    // 10004
    events[4].probableCause="MEMORY ALLOCATION FAILURE";
    events[4].objectOfRef="FE/";

    // 10005
    events[5].probableCause="UNEXPECTED INTERNAL FAILURE";
    events[5].objectOfRef="FE/";

    // 10006
    events[6].probableCause="SERVER ACCEPT FAILURE";
    events[6].objectOfRef="FE/DSD";

    // 10007
    events[7].probableCause="THIS SHOULD NOT HAPPEN";
    events[7].objectOfRef="FE/";

    // 10008
    events[8].probableCause="PRIMITIVE FAILURE";
    events[8].objectOfRef="FE/PH";

    // 10009
    events[9].probableCause="PROGRAM FAILURE";
    events[9].objectOfRef="FE/";

    // 10010
    events[10].probableCause="FILE FAILURE";
    events[10].objectOfRef="FE/SysDisk";

    // 10011
    events[11].probableCause="ECHO TIMEOUT";
    events[11].objectOfRef="FE/";

    // 10012
    events[12].probableCause="CLIENT CONNECT FAILURE";
    events[12].objectOfRef="NFE/DSD";

    // 10013
    events[13].probableCause="SENDING DATA FAILURE";
    events[13].objectOfRef="FE/PH";

    // 10014
    events[14].probableCause="RECEIVING DATA FAILURE";
    events[14].objectOfRef="FE/PH";

    // 10015
    events[15].probableCause="OPEN UDP SOCKET FAILURE";
    events[15].objectOfRef="FE/";

    // 10016
    events[16].probableCause="EVENT RESET";
    events[16].objectOfRef="FE/";

    // 10017
    events[17].probableCause="REMOTE SIDE DISCONNECTED";
    events[17].objectOfRef="FE/";

    // 10018
    events[18].probableCause="ACS_CS_API_NetworkElement::isMultipleCPSystem Error";
    events[18].objectOfRef="FE/CS";


    // A L A R M , AP CP CLUSTER COMMUNICATION FAULT
    // 10099, ALARM number
    events[99].probableCause="AP CP CLUSTER COMMUNICATION FAULT";
    events[99].problemData="Echo signalling on IP-address(es) broken";
}

//*************************************************************************
// Destructor
//*************************************************************************
TOCAP_Events::~TOCAP_Events()
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
void TOCAP_Events::reportEvent(int problemNo,string probData,string objRef)
{
    newTRACE((LOG_LEVEL_INFO,"TOCAP_Events::reportEvent()",0));

    // in order to prevent flooding of the event viewer the same
    // event is not reported within a 10 minute interval.

    bool reportEvent = !(this->tooFrequent(problemNo));
   
    if (reportEvent)
    {
        stringstream ss;
        ss<<probData.c_str()<<"\noccurred "<<eventCnt[problemNo-10000]
        <<" times during suppression";
        probData=ss.str();

       if (objRef.compare("")==0)
       {
           objRef=events[problemNo-10000].objectOfRef;
       }

       ACS_AEH_ReturnType err = ACS_AEH_ok;
       if ( reportEvent )
       {
            eventCnt[problemNo-10000]=0;

            // check return code of this method!!!!!
            err = evp->sendEventMessage(procName.c_str(),
                           problemNo,
                           "EVENT",
                           events[problemNo-10000].probableCause.c_str(),
                           "APZ",
                           objRef.c_str(),
                           probData.c_str(),
                           events[problemNo-10000].problemText.c_str());

       }

        ostringstream trace;
        if (ACS_AEH_error == err)
        {
            ACS_AEH_ErrorType errCode = evp->getError();
            const char* errorText = evp->getErrorText();
            trace << endl << "TOCAP_Events::reportEvent failed" <<endl;
            trace << "ACS_AEH_EvReport::sendEventMessage failed" << endl;
            trace << "Error Code = " << errCode << endl;
            trace << "Error Test = \"" << errorText << "\"" << endl;
        }
        else
        {
            trace << endl << "TOCAP_Events::reportEvent" << endl;
        }

        trace << "EVENT = " << problemNo << endl;
        trace << events[problemNo-10000].probableCause.c_str() << endl;
        trace << objRef.c_str() << endl;
        trace << probData.c_str() << endl;
        TRACE((LOG_LEVEL_INFO,"%s",0,trace.str().c_str()));

    }
}

//*************************************************************************
// alarmUpdate
//
// Issue/Cease alarm 'AP CP CLUSTER COMMUNICATION FAULT'.
//
// return value:
//   -
//*************************************************************************
void TOCAP_Events::alarmUpdate(string alarmaction, TOCAP::TypeOfAlarm toa, string nodeName, vector<string> ip, int net)
{

    newTRACE((LOG_LEVEL_INFO,"TOCAP_Events::alarmUpdate()",0));

    ostringstream probText;
    probText<<"FAULT"<<endl;
    if (toa==TOCAP::LINK_ALARM)
    {
        if(net == 0)
            events[99].objectOfRef="LINK_A/";
        else
            events[99].objectOfRef="LINK_B/";
        probText<<"LINK";
    }
    else
    {
        events[99].objectOfRef="UNIT/";
        probText<<"UNIT";
    }
    events[99].objectOfRef +=nodeName;
    probText<<endl<<endl<<"UNITNAME"<<setw(18)<<"IPADDRESS"<<endl;
    probText<<nodeName;
    int ipOffset=0;
    for (vector<string>::iterator it=ip.begin();it!=ip.end();++it)
    {
        ipOffset=17 + (int)(*it).size();
        if (it==ip.begin()) ipOffset -= (int)nodeName.size();
        probText<<setw(ipOffset)<<(*it)<<endl;
    }

    string probData="";
    if (ip.size()==0) probData="alarm ceased at process start/stop";
    else probData=events[99].problemData;

    ACS_AEH_ReturnType err = evp->sendEventMessage(const_cast<char*>(procName.c_str()),
                     10099,
                     alarmaction.c_str(),
                     events[99].probableCause.c_str(),
                     "APZ",
                     events[99].objectOfRef.c_str(),
                     probData.c_str(),
                     probText.str().c_str(),
                     true); // allow manual cease


    ostringstream trace;
    if (ACS_AEH_error == err)
    {
        ACS_AEH_ErrorType errCode = evp->getError();
        const char* errorText = evp->getErrorText();
        trace << endl << "TOCAP_Events::alarmUpdate failed!" << endl;
        trace << "ACS_AEH_EvReport::sendEventMessage failed" << endl;
        trace << "Error Code = " << errCode << endl;
        trace << "Error Test = \"" << errorText << "\"" << endl;
    }
    else
    {
        trace << endl << "TOCAP_Events::alarmUpdate" << endl;
    }

    trace << "ALARM = 10099 " << alarmaction.c_str() << endl;
    trace << events[99].probableCause.c_str() << endl;
    trace << events[99].objectOfRef.c_str() << endl;
    trace << probData.c_str() << endl;
    trace << probText.str().c_str() << endl;
    TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));
}


//*************************************************************************
// tooFrequent
//
// Checks whether this particular event has been reported recently (within
// the last 100 seconds) and if so, suppress the event reporting.
//
// return value:
//   false : not too frequent, go ahead and report event.
//   true  : suppress event.
//*************************************************************************
bool TOCAP_Events::tooFrequent(int evNum)
{
    bool retCode=false;

    // event 10011 should not be suppressed.
    if (( 10011 == evNum ) || ( 10017 == evNum ))
    {
       return retCode;
    }

    try
    {
        time_t rightNow(0);
        time(&rightNow);
        if (eventTime[evNum-10000]!=0 && (rightNow-eventTime[evNum-10000])<100)
        {
            // suppress the event.
            eventCnt[evNum-10000]++;
            retCode=true;
        }
        else
        {
            eventTime[evNum-10000]=rightNow;
        }
    }
    catch (...)
    {
        retCode=false;
    }
    return retCode;
}




