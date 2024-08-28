//******************************************************************************
// NAME
// OCS_OCP_Service.h
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

#include "OCS_OCP_Service.h"
#include "OCS_OCP_Trace.h"


/*
 * Description: This is the class constructor.
 * @para[in]: daemon_name - name of application.
 * @para[in]: user_name - user is used to start application.
 * @return: N/A
 */
OCS_OCP_Service::OCS_OCP_Service(const char* daemon_name, const char* user_name):ACS_APGCC_ApplicationManager(daemon_name, user_name)
{
    m_appThread = 0;
    m_ocsOcpServer = 0;
    m_checkApNodeStateThread = 0;
    m_checkApNodeState = 0;
    m_hwtableObserver = 0;
    m_isAppStarted = false;

}

/*
 * Description: This is the class destructor.
 * @para: N/A
 * @return: N/A
 */
OCS_OCP_Service::~OCS_OCP_Service()
{
    if(m_ocsOcpServer != 0)
    {
        delete m_ocsOcpServer;
        m_ocsOcpServer = 0;
    }

    if(m_checkApNodeState != 0)
    {
        delete m_checkApNodeState;
        m_checkApNodeState = 0;
    }

    if(this->m_appThread != 0)
    {
        delete m_appThread;
        m_appThread = 0;
    }


    if(this->m_checkApNodeStateThread != 0)
    {
        delete m_checkApNodeStateThread;
        m_checkApNodeStateThread = 0;
    }

    if(m_hwtableObserver != 0)
    {
        m_hwtableObserver->unsubscribeHWCTableChanges();
        delete m_hwtableObserver;
        m_hwtableObserver = 0;
    }
}
/*
 * Description: This function is used to activate amf service loop.
 * @para: N/A
 * @return: The error code
 */
ACS_APGCC_HA_ReturnType OCS_OCP_Service::amfInitialize()
{
    newTRACE((LOG_LEVEL_INFO,"OCS_OCP_Service::amfInitialize()",0));

    ACS_APGCC_HA_ReturnType errorCode = ACS_APGCC_HA_SUCCESS;

    errorCode = this->activate();

    if (errorCode == ACS_APGCC_HA_FAILURE)
    {
        TRACE((LOG_LEVEL_ERROR, "ocs_tocapd. HA Activation Failed",0));
    }

    if (errorCode == ACS_APGCC_HA_FAILURE_CLOSE)
    {
        TRACE((LOG_LEVEL_ERROR, "ocs_tocapd, HA Application Failed to Gracefully closed!!",0));
    }

    if (errorCode == ACS_APGCC_HA_SUCCESS)
    {
        TRACE((LOG_LEVEL_INFO,"ocs_tocapd, HA Application Gracefully closed!!", 0));
    }

    return errorCode;
}

/*
 * Description: This functions is used to run the application.
 * @para: N/A
 * @return: N/A
 */
void OCS_OCP_Service::run()
{
    //Start application thread
    this->m_ocsOcpServer = new OCS_OCP_Server(this);
    m_appThread = new  boost::thread(boost::bind(&OCS_OCP_Server::run, m_ocsOcpServer));

    //Start application thread
    this->m_checkApNodeState = new OCS_OCP_CheckAPNodeState();
    m_checkApNodeStateThread = new  boost::thread(boost::bind(&OCS_OCP_CheckAPNodeState::run, m_checkApNodeState));

    //Subscribe the HW table change notification
    m_hwtableObserver = new OCS_OCP_HWCTableObserver();
    if(m_hwtableObserver != 0)
        m_hwtableObserver->subscribeHWCTableChanges();

}

/*
 * Description: This functions is used to stop application.
 * @para: N/A
 * @return: N/A
 */
void OCS_OCP_Service::stop()
{
    if(this->m_ocsOcpServer != 0)
        this->m_ocsOcpServer->stop();

    if(this->m_checkApNodeState != 0)
           this->m_checkApNodeState->stop();

    // waiting for application thread exits.
    if((this->m_appThread != 0) && (this->m_appThread->joinable()))
        this->m_appThread->join();

    // waiting for application thread exits.
    if((this->m_checkApNodeStateThread != 0) && (this->m_checkApNodeStateThread->joinable()))
       this->m_checkApNodeStateThread->join();

    if(m_ocsOcpServer != 0)
    {
       delete m_ocsOcpServer;
       m_ocsOcpServer = 0;
    }

    if(m_checkApNodeState != 0)
    {
       delete m_checkApNodeState;
       m_checkApNodeState = 0;
    }

    if(this->m_appThread != 0)
    {
           delete m_appThread;
           m_appThread = 0;
    }


    if(this->m_checkApNodeStateThread != 0)
    {
           delete m_checkApNodeStateThread;
           m_checkApNodeStateThread = 0;
    }

    if(m_hwtableObserver != 0)
    {
            m_hwtableObserver->unsubscribeHWCTableChanges();
            delete m_hwtableObserver;
            m_hwtableObserver = 0;
    }
}

/*
 * Description: This is a callback functions that is called from AMF when application's state changes to active.
 * @para[in]: previousHAState - the previous state of application
 * @return: ACS_APGCC_SUCCESS
 */
ACS_APGCC_ReturnType OCS_OCP_Service::performStateTransitionToActiveJobs(ACS_APGCC_AMF_HA_StateT previousHAState)
{
    /* Check if we have received the ACTIVE State Again.
     * This means that, our application is already Active and
     * again we have got a callback from AMF to go active.
     * Ignore this case anyway. This case should rarely happens
     */

    newTRACE((LOG_LEVEL_INFO, "OCS_OCP_Service::performStateTransitionToActiveJobs()",0));

    if(ACS_APGCC_AMF_HA_ACTIVE == previousHAState)
        return ACS_APGCC_SUCCESS;

    /* Our application has received state ACTIVE from AMF.
     * Start off with the activities needs to be performed
     * on ACTIVE
     */

    /* Handle here what needs to be done when you are given ACTIVE State */
    TRACE((LOG_LEVEL_INFO,"My Application Component received ACTIVE state assignment!!!", 0));

    // Run application.
    if(!m_isAppStarted)
    {
        TRACE((LOG_LEVEL_INFO, "Start application", 0));

        this->run();
        m_isAppStarted = true;
    }

    return ACS_APGCC_SUCCESS;


}

/*
 * Description: This is a callback functions that is called from AMF when application's state changes to queising.
 * @para[in]: previousHAState - the previous state of application
 * @return: ACS_APGCC_SUCCESS
 */
ACS_APGCC_ReturnType OCS_OCP_Service::performStateTransitionToQueisingJobs(ACS_APGCC_AMF_HA_StateT /*previousHAState*/)
{
    /* We were active and now losing active state due to some shutdown admin
     * operation performed on our SU.
     * Inform the thread to go to "stop" state
     */
    newTRACE((LOG_LEVEL_INFO,"OCS_OCP_Service::performStateTransitionToQueisingJobs()",0));

    TRACE((LOG_LEVEL_INFO,"My Application Component received QUIESING state assignment!!!", 0));

    // Stop application.
    if(m_isAppStarted)
    {
        TRACE((LOG_LEVEL_INFO,"Stop application", 0));

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
ACS_APGCC_ReturnType OCS_OCP_Service::performStateTransitionToQuiescedJobs(ACS_APGCC_AMF_HA_StateT /*previousHAState*/)
{
    newTRACE((LOG_LEVEL_INFO, "OCS_OCP_Service::performStateTransitionToQuiescedJobs()",0));

    /* We were Active and now losting Active state due to Lock admin
     * operation performed on our SU.
     * Inform the thread to go to "stop" state
     */

    TRACE((LOG_LEVEL_INFO,"My Application Component received QUIESCED state assignment!", 0));

    // Stop application.
    if(m_isAppStarted)
    {
        TRACE((LOG_LEVEL_INFO,"Stop application", 0));

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
ACS_APGCC_ReturnType OCS_OCP_Service::performComponentHealthCheck()
{
    //newTRACE((LOG_LEVEL_INFO,"OCS_OCP_Service::performComponentHealthCheck()", 0));

    ACS_APGCC_ReturnType rc = ACS_APGCC_SUCCESS;

    return rc;
}

/*
 * Description: This is a callback functions that is called from AMF when application is terminated.
 * @para: N/A
 * @return: ACS_APGCC_SUCCESS
 */
ACS_APGCC_ReturnType OCS_OCP_Service::performComponentTerminateJobs(void)
{
    newTRACE((LOG_LEVEL_INFO,"OCS_OCP_Service::performComponentTerminateJobs()",0));

    TRACE((LOG_LEVEL_INFO,"My Application Component received terminate callback!", 0));

    // Stop application.
    if(m_isAppStarted)
    {
        TRACE((LOG_LEVEL_INFO,"Stop application", 0));

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
ACS_APGCC_ReturnType OCS_OCP_Service::performComponentRemoveJobs (void)
{
    newTRACE((LOG_LEVEL_INFO,"OCS_OCP_Service::performComponentRemoveJobs()",0));

    /* Application has received Removal callback. State of the application
     * is neither Active nor Standby. This is with the result of LOCK admin operation
     * performed on our SU. Terminate the thread by informing the thread to go "stop" state.
     */

    TRACE((LOG_LEVEL_INFO,"Application Assignment is removed now", 0));

    // Stop application.
    if(m_isAppStarted)
    {
        TRACE((LOG_LEVEL_INFO,"Stop application", 0));

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
ACS_APGCC_ReturnType OCS_OCP_Service::performApplicationShutdownJobs(void)
{
    newTRACE((LOG_LEVEL_INFO,"OCS_OCP_Service::performApplicationShutdownJobs",0));

    TRACE((LOG_LEVEL_INFO,"Shutting down the application", 0));

    // Stop application.
    if(m_isAppStarted)
    {
        TRACE((LOG_LEVEL_INFO,"Stop application", 0));

        this->stop();
        m_isAppStarted = false;

    }

    return ACS_APGCC_SUCCESS;
}

