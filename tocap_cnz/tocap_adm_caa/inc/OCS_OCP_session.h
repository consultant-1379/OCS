//******************************************************************************
// NAME
// OCS_OCP_session.h
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
// 2008-07-16 by EAB/FTE/DDH UABCAJN

// DESCRIPTION
// This class is an abstract base class.
// An HB_Session object supervises the state of a CP-AP echo session. The
// object is updated by the echo session object, which sends/receives echo
// data, about the echo session status. The HB_Session object supervises
// that the echo updates arrives regularly, and if not, raises a named
// event that the particular CP port is down.

// LATEST MODIFICATION
// -
//******************************************************************************
#ifndef _OCS_OCP_session_h
#define _OCS_OCP_session_h

#include "OCS_OCP_global.h"
#include "OCS_OCP_events.h"
#include <sstream>
#include <string>
#include <fstream>
#include <time.h>
#include <stdint.h>

const int ALARM_SUPPRESSION_TIME=120;
const int HB_TIMEOUT=30;

class HB_Session
{
public:
	HB_Session(TOCAP_Events* ev);
	virtual ~HB_Session();
	virtual void operator()(std::ofstream& of,uint32_t& recNr);
	virtual bool isObjWithIP(uint32_t ip1,uint32_t ip2);
	virtual bool isObjWithIP(const uint32_t cpip);
	virtual void clockTick(void)=0;
	virtual bool heartBeat(char spxside);
	virtual void updateState(TOCAP::SessionState s_otherAP);
	virtual TOCAP::SessionState getSessionState(void) const;
	virtual TOCAP::SpxState getSpxState(void) const;
	virtual time_t getLastHeartBeat(void) const;
    virtual void toString(std::ostringstream& of);
	static int failoverSuppression; 	

	void signalSessionDown(std::string cpDotAddr);
	void signalSessionUp(std::string cpDotAddr);
	virtual void signalSessionDisconnect(void)=0;
	bool isHeartBeatTimeOut();

protected:
	TOCAP::SessionState s_state;
	char spxSide; // 2=EX side, 3=SB side 
	time_t lastHeartBeat;
	time_t eventHandleTime;
	TOCAP_Events* evRep;

	// used by the logging function to avoid logging the same data
	bool attributesUpdated; 

	bool heartBeatTimeOut;
};

#endif
