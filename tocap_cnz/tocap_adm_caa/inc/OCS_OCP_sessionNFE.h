//******************************************************************************
// NAME
// OCS_OCP_sessionNFE.h
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
// 2008-02-28 by EAB/FTE/DDH UABCAJN

// DESCRIPTION
// This class supervises echo sessions on non front end (NFE) AP's (including
// the passive node on the front end AP system).

// LATEST MODIFICATION
// -
//******************************************************************************
#ifndef _OCS_OCP_sessionNFE_h
#define _OCS_OCP_sessionNFE_h

#include "OCS_OCP_session.h"
#include <string>
#include <time.h>

const int NOTIFICATION_INTERVAL=60;
const int MIN_NOTIFICATION_INTERVAL=20;

#include "OCS_OCP_protHandler.h"
#include "OCS_OCP_events.h"

//#include "ACS_DSD_Client.h"

class HB_Session_NFE :public HB_Session
{
public:
	HB_Session_NFE(bool multiCP,uint32_t apip,uint32_t cpip,
		TOCAP_Events* evH);
	~HB_Session_NFE();
	void operator()(std::ofstream& of,unsigned int& recNr);
	bool isObjWithIP(const uint32_t ip);
	void clockTick(void);
	bool heartBeat(char latestSpxSide);
    void toString(std::ostringstream& of);
	void signalSessionUp();
	void signalSessionDisconnect(void);

private:
	bool notifyFrontEndAP();
	std::string ulong_to_dotIP(uint32_t ip);
	void checkFrontEndNames(std::vector<std::string>& fe_names);

	bool multiCPsystem;
	static std::string frontEndNodeName; // when connecting to FE, try this "cached" name first. 
	
	uint32_t ap_ip;
	uint32_t cp_ip;
	bool resendNotification;
	time_t lastNotificationSent;	
};

#endif
