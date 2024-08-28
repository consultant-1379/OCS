//******************************************************************************
// COPYRIGHT Ericsson Utvecklings AB, Sweden 2000.
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
// OCS_IPN_Service.cpp
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


#include "OCS_IPN_Service.h"
#include "OCS_IPN_Common.h"
#include "OCS_IPN_Trace.h"


/*
 * Description: This is the class constructor.
 * @para[in]: daemon_name - name of application.
 * @para[in]: user_name - user is used to start application.
 * @return: N/A
 */
OCS_IPN_Service::OCS_IPN_Service(const char* daemon_name, const char* user_name):ACS_APGCC_ApplicationManager(daemon_name, user_name)
{
	m_appThread = 0;
	m_cpRelatedSwManagerOIThread = 0;
	m_ocsIPNServer = 0;
	m_cpRelatedSwManagerOI = 0;
	m_isAppStarted = false;

}

/*
 * Description: This is the class destructor.
 * @para: N/A
 * @return: N/A
 */
OCS_IPN_Service::~OCS_IPN_Service()
{

    if(m_cpRelatedSwManagerOI != 0)
    {
       delete m_cpRelatedSwManagerOI;
       m_cpRelatedSwManagerOI = 0;
    }

    if(m_ocsIPNServer != 0)
    {
        delete m_ocsIPNServer;
        m_ocsIPNServer = 0;
    }

    if(this->m_cpRelatedSwManagerOIThread != 0)
    {
           delete m_cpRelatedSwManagerOIThread;
           m_cpRelatedSwManagerOIThread = 0;
    }


    if(this->m_appThread != 0)
    {
            delete m_appThread;
            m_appThread = 0;
    }

}
/*
 * Description: This function is used to activate amf service loop.
 * @para: N/A
 * @return: The error code
 */
ACS_APGCC_HA_ReturnType OCS_IPN_Service::amfInitialize()
{

    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_Service::amfInitialize()",0));

	ACS_APGCC_HA_ReturnType errorCode = ACS_APGCC_HA_SUCCESS;

	errorCode = this->activate();

	if (errorCode == ACS_APGCC_HA_FAILURE)
	{
	    TRACE((LOG_LEVEL_INFO, "ocs_ipnaadmd. HA Activation Failed",0));
	}

	if (errorCode == ACS_APGCC_HA_FAILURE_CLOSE)
	{
	    TRACE((LOG_LEVEL_INFO, "ocs_ipnaadmd, HA Application Failed to Gracefully closed!!",0));
	}

	if (errorCode == ACS_APGCC_HA_SUCCESS)
	{
	    TRACE((LOG_LEVEL_INFO, "ocs_ipnaadmd, HA Application Gracefully closed!!",0));
	}

	return errorCode;
}

/*
 * Description: This functions is used to run the application.
 * @para: N/A
 * @return: N/A
 */
void OCS_IPN_Service::start()
{
  	//Start application thread
    this->m_ocsIPNServer = new OCS_IPN_Server(this);
	m_appThread = new  boost::thread(boost::bind(&OCS_IPN_Server::start, m_ocsIPNServer));

	//Start CpRelatedSwManage OI Thread
    this->m_cpRelatedSwManagerOI = new OCS_IPN_CpRelatedSwManagerOI(OCS_IPN_Common::IMM_CPRELATEDMANAGER_IMPL_NAME);
    m_cpRelatedSwManagerOIThread = new  boost::thread(boost::bind(&OCS_IPN_CpRelatedSwManagerOI::run, m_cpRelatedSwManagerOI));
}

/*
 * Description: This functions is used to stop application.
 * @para: N/A
 * @return: N/A
 */
void OCS_IPN_Service::stop()
{
    newTRACE((LOG_LEVEL_INFO, "Enter OCS_IPN_Service::stop()",0));

    if(this->m_cpRelatedSwManagerOI != 0)
               this->m_cpRelatedSwManagerOI->stop();

    if(this->m_ocsIPNServer != 0)
    	this->m_ocsIPNServer->stop();

    // waiting for application thread exits.
    if(this->m_cpRelatedSwManagerOIThread != 0)
        this->m_cpRelatedSwManagerOIThread->join();

    if(this->m_appThread != 0)
    	this->m_appThread->join();


    if(m_cpRelatedSwManagerOI != 0)
    {
       delete m_cpRelatedSwManagerOI;
       m_cpRelatedSwManagerOI = 0;
    }

    if(m_ocsIPNServer != 0)
        {
            delete m_ocsIPNServer;
            m_ocsIPNServer = 0;
        }


	if(this->m_cpRelatedSwManagerOIThread != 0)
    {
           delete m_cpRelatedSwManagerOIThread;
           m_cpRelatedSwManagerOIThread = 0;
    }

	if(this->m_appThread != 0)
	    {
	            delete m_appThread;
	            m_appThread = 0;
	    }

	 TRACE((LOG_LEVEL_INFO, "Exit OCS_IPN_Service::stop()",0));
}

/*
 * Description: This is a callback functions that is called from AMF when application's state changes to active.
 * @para[in]: previousHAState - the previous state of application
 * @return: ACS_APGCC_SUCCESS
 */
ACS_APGCC_ReturnType OCS_IPN_Service::performStateTransitionToActiveJobs(ACS_APGCC_AMF_HA_StateT previousHAState)
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_Service::performStateTransitionToActiveJobs",0));

    /* Check if we have received the ACTIVE State Again.
	 * This means that, our application is already Active and
	 * again we have got a callback from AMF to go active.
	 * Ignore this case anyway. This case should rarely happens
	 */

	if(ACS_APGCC_AMF_HA_ACTIVE == previousHAState)
		return ACS_APGCC_SUCCESS;

	/* Our application has received state ACTIVE from AMF.
	 * Start off with the activities needs to be performed
	 * on ACTIVE
	 */

	/* Handle here what needs to be done when you are given ACTIVE State */
	TRACE((LOG_LEVEL_INFO, "My Application Component received ACTIVE state assignment!!!",0));

	// Run application.
	if(!m_isAppStarted)
	{
        TRACE((LOG_LEVEL_INFO, "Start application",0));
        this->start();
        m_isAppStarted = true;
	}

	return ACS_APGCC_SUCCESS;
}

ACS_APGCC_ReturnType OCS_IPN_Service::performStateTransitionToPassiveJobs(ACS_APGCC_AMF_HA_StateT previousHAState)
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_Service::performStateTransitionToPassiveJobs",0));

    TRACE((LOG_LEVEL_INFO, "My Application Component received Passive state assignment!!!",0));

    return ACS_APGCC_SUCCESS;
}

/*
 * Description: This is a callback functions that is called from AMF when application's state changes to queising.
 * @para[in]: previousHAState - the previous state of application
 * @return: ACS_APGCC_SUCCESS
 */
ACS_APGCC_ReturnType OCS_IPN_Service::performStateTransitionToQueisingJobs(ACS_APGCC_AMF_HA_StateT /*previousHAState*/)
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_Service::performStateTransitionToQueisingJobs",0));
	/* We were active and now losing active state due to some shutdown admin
	 * operation performed on our SU.
	 * Inform the thread to go to "stop" state
	 */

    TRACE((LOG_LEVEL_INFO, "My Application Component received QUIESING state assignment!!!",0));

    // Stop application.
	if(m_isAppStarted)
	{
	    TRACE((LOG_LEVEL_INFO, "Stop application",0));
		this->stop();
		m_isAppStarted = false;
	}

	return ACS_APGCC_SUCCESS;
}

/*
 * Description: This is a callback functions that is called from AMF when application's state changes to queisced.
 * @para[in]: previousHAState - the previous state of application
 * @return: ACS_APGCC_SUCCESS
 */
ACS_APGCC_ReturnType OCS_IPN_Service::performStateTransitionToQuiescedJobs(ACS_APGCC_AMF_HA_StateT /*previousHAState*/)
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_Service::performStateTransitionToQuiescedJobs",0));

	/* We were Active and now losting Active state due to Lock admin
	 * operation performed on our SU.
	 * Inform the thread to go to "stop" state
	 */

	TRACE((LOG_LEVEL_INFO, "My Application Component received QUIESCED state assignment!",0));

	// Stop application.
	if(m_isAppStarted)
	{
	    TRACE((LOG_LEVEL_INFO, "Stop application",0));
		this->stop();
		m_isAppStarted = false;
	}

	return ACS_APGCC_SUCCESS;
}

/*
 * Description: This is a callback functions that is called from AMF in every healthcheck interval.
 * @para: N/A
 * @return: ACS_APGCC_SUCCESS
 */
ACS_APGCC_ReturnType OCS_IPN_Service::performComponentHealthCheck()
{
    newTRACE((LOG_LEVEL_TRACE, "OCS_IPN_Service::performComponentHealthCheck",0));

	ACS_APGCC_ReturnType rc = ACS_APGCC_SUCCESS;

    return rc;
}

/*
 * Description: This is a callback functions that is called from AMF when application is terminated.
 * @para: N/A
 * @return: ACS_APGCC_SUCCESS
 */
ACS_APGCC_ReturnType OCS_IPN_Service::performComponentTerminateJobs(void)
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_Service::performComponentTerminateJobs",0));

    TRACE((LOG_LEVEL_INFO, "My Application Component received terminate callback!",0));

    // Stop application.
	if(m_isAppStarted)
	{
	    TRACE((LOG_LEVEL_INFO, "Stop application" ,0));
		this->stop();
		m_isAppStarted = false;
	}
	return ACS_APGCC_SUCCESS;
}

/*
 * Description: This is a callback functions that is called from AMF when application is stopped.
 * @para: N/A
 * @return: ACS_APGCC_SUCCESS
 */
ACS_APGCC_ReturnType OCS_IPN_Service::performComponentRemoveJobs (void)
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_Service::performComponentRemoveJobs",0));

	/* Application has received Removal callback. State of the application
	 * is neither Active nor Standby. This is with the result of LOCK admin operation
	 * performed on our SU. Terminate the thread by informing the thread to go "stop" state.
	 */

	TRACE((LOG_LEVEL_INFO, "Application Assignment is removed now",0));

	// Stop application.
	if(m_isAppStarted)
	{
	    TRACE((LOG_LEVEL_INFO, "Stop application",0));
		this->stop();
		m_isAppStarted = false;

	}

	return ACS_APGCC_SUCCESS;
}

/*
 * Description: This is a callback functions that is called from AMF when shutting down AMF.
 * @para: N/A
 * @return: ACS_APGCC_SUCCESS
 */
ACS_APGCC_ReturnType OCS_IPN_Service::performApplicationShutdownJobs(void)
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_Service::performApplicationShutdownJobs",0));

    TRACE((LOG_LEVEL_INFO, "Shutting down the application",0));

	// Stop application.
	if(m_isAppStarted)
	{
		TRACE((LOG_LEVEL_INFO, "Stop application",0));
		this->stop();
		m_isAppStarted = false;

	}

	return ACS_APGCC_SUCCESS;
}

