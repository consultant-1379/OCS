//******************************************************************************
// NAME
// OCS_OCP_sessionFEO.h
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
// 2007-11-26 by EAB/FTE/DDH UABCAJN

// DESCRIPTION
// This class supervises echo sessions which exist on a different AP, However
// the class object is instantiated by the front end AP. FEO should be
// interpreted as Front End Other(AP).

// LATEST MODIFICATION
// -
//******************************************************************************
#ifndef _OCS_OCP_sessionFEO_h
#define _OCS_OCP_sessionFEO_h

#include "OCS_OCP_session.h"
#include <vector>
#include <time.h>

class Link;
class HB_Session_FEO :public HB_Session
{
public:
	HB_Session_FEO(Link* l1,Link* l2,TOCAP_Events* ev);
	void operator()(std::ofstream& of,unsigned int& recNr);
	bool isObjWithIP(uint32_t ip1,uint32_t ip2);
	bool isObjWithIP(const uint32_t apip);
	void clockTick(void);
	void updateState(TOCAP::SessionState s_otherAP);
    void toString(std::ostringstream& of);
	void signalSessionDisconnect(void){};

private:
	std::vector<Link*> links;
	time_t pendingStart;
	time_t checkStatusTime;

};

#endif
