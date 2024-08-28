//******************************************************************************
	// COPYRIGHT Ericsson Utvecklings AB, Sweden 2011.
// All rights reserved.
//
// The Copyright to the computer program(s) herein 
// is the property of Ericsson Utvecklings AB, Sweden.
// The program(s) may be used and/or copied only with 
// the written permission from Ericsson Utvecklings AB or in 
// accordance with the terms and conditions stipulated in the 
// agreement/contract under which the program(s) have been 
// supplied.
// 
// NAME
// OCS_IPN_Service.h
//
// DESCRIPTION
// This file is used for the ipn server class implementation.
//
// DOCUMENT NO
// 190 89-CAA 109 1405
//
// AUTHOR
// XDT/DEK XTUANGU
//
//******************************************************************************
// *** Revision history ***
// 2011-07-14 Created by XTUANGU
//******************************************************************************

#ifndef OCS_IPN_Service_h
#define OCS_IPN_Service_h


#include <ACS_APGCC_ApplicationManager.h>
#include "OCS_IPN_Server.h"
#include "OCS_IPN_CpRelatedSwManagerOI.h"
#include <boost/thread.hpp>

class OCS_IPN_Service : public ACS_APGCC_ApplicationManager
{
public:
    OCS_IPN_Service(const char* daemon_name, const char* user_name);
	virtual ~OCS_IPN_Service();

	ACS_APGCC_HA_ReturnType amfInitialize();

	ACS_APGCC_ReturnType performStateTransitionToActiveJobs(ACS_APGCC_AMF_HA_StateT previousHAState);
	ACS_APGCC_ReturnType performStateTransitionToPassiveJobs(ACS_APGCC_AMF_HA_StateT previousHAState);
	ACS_APGCC_ReturnType performStateTransitionToQueisingJobs(ACS_APGCC_AMF_HA_StateT previousHAState);
	ACS_APGCC_ReturnType performStateTransitionToQuiescedJobs(ACS_APGCC_AMF_HA_StateT previousHAState);
	ACS_APGCC_ReturnType performComponentHealthCheck(void);
	ACS_APGCC_ReturnType performComponentTerminateJobs(void);
	ACS_APGCC_ReturnType performComponentRemoveJobs (void);
	ACS_APGCC_ReturnType performApplicationShutdownJobs(void);

private:

	void start();
	void stop();

	bool m_isAppStarted;
	boost::thread* m_appThread;
	boost::thread* m_cpRelatedSwManagerOIThread;

	OCS_IPN_Server* m_ocsIPNServer;

	OCS_IPN_CpRelatedSwManagerOI* m_cpRelatedSwManagerOI;

};
#endif
