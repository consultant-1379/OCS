//******************************************************************************
// NAME
// OCS_OCP_sessionFE.h
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
// 2007-10-15 by EAB/FTE/DDH UABCAJN

// DESCRIPTION
// This class supervises echo sessions on the front end (FE) AP.

// LATEST MODIFICATION
// -
//******************************************************************************
#ifndef _OCS_OCP_sessionFE_h
#define _OCS_OCP_sessionFE_h

#include "OCS_OCP_session.h"
#include <vector>
#include <time.h>
#include "OCS_OCP_link.h"
#include "OCS_OCP_events.h"

class HB_Session_FE :public HB_Session
{
public:
	HB_Session_FE(Link* l1,Link* l2,TOCAP_Events* ev);
	void operator()(std::ofstream& of,unsigned int& recNr);
	bool isObjWithIP(const uint32_t ip);
	void clockTick(void);
	bool heartBeat(char latestSpxSide);
    void toString(std::ostringstream& of);
	void signalSessionUp();
	void signalSessionDisconnect(void);

private:
	std::vector<Link*> links;
	time_t pendingStart;
    time_t checkStatusTime;

};

#endif
