//******************************************************************************
// NAME
// OCS_OCP_Service.cpp
//
// COPYRIGHT Ericsson AB, Sweden 2010.
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
// ???

// AUTHOR
// 2010-12-01 by XDT/DEK XTUANGU

// DESCRIPTION
// This class is use for presenting the AMF service.

// LATEST MODIFICATION
// -
//******************************************************************************

#ifndef OCS_OCP_Service_H_
#define OCS_OCP_Service_H_


#include <ACS_APGCC_ApplicationManager.h>
#include "OCS_OCP_Server.h"
#include "OCS_OCP_CheckAPNodeState.h"
#include "OCS_OCP_HWCTableObserver.h"
#include <boost/thread.hpp>

class OCS_OCP_Service : public ACS_APGCC_ApplicationManager
{
public:
    OCS_OCP_Service(const char* daemon_name, const char* user_name);
	virtual ~OCS_OCP_Service();

	ACS_APGCC_HA_ReturnType amfInitialize();

	ACS_APGCC_ReturnType performStateTransitionToActiveJobs(ACS_APGCC_AMF_HA_StateT previousHAState);
	ACS_APGCC_ReturnType performStateTransitionToQueisingJobs(ACS_APGCC_AMF_HA_StateT previousHAState);
	ACS_APGCC_ReturnType performStateTransitionToQuiescedJobs(ACS_APGCC_AMF_HA_StateT previousHAState);
	ACS_APGCC_ReturnType performComponentHealthCheck(void);
	ACS_APGCC_ReturnType performComponentTerminateJobs(void);
	ACS_APGCC_ReturnType performComponentRemoveJobs (void);
	ACS_APGCC_ReturnType performApplicationShutdownJobs(void);

private:

	void run();
	void stop();

	bool m_isAppStarted;
	boost::thread* m_appThread;
	OCS_OCP_Server* m_ocsOcpServer;

	boost::thread* m_checkApNodeStateThread;
	OCS_OCP_CheckAPNodeState* m_checkApNodeState;
	OCS_OCP_HWCTableObserver* m_hwtableObserver;


};

#endif /* OCS_OCP_Service_H_ */
