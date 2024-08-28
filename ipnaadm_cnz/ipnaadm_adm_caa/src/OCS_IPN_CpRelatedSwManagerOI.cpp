//******************************************************************************
// COPYRIGHT Ericsson Utvecklings AB, Sweden 2012.
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
// OCS_IPN_Common.cpp
//
// DESCRIPTION
// Methods implementation for class OCS_IPN_CpRelatedSwManagerOI
//
// DOCUMENT NO
// 190 89-CAA 109 1405
//
// AUTHOR
// XDT/DEK XTUANGU
//
//******************************************************************************
// *** Revision history ***
// 2012-07-10 Created by XTUANGU
//******************************************************************************


#include "OCS_IPN_CpRelatedSwManagerOI.h"
#include "OCS_IPN_Trace.h"
#include "OCS_IPN_Thread.h"

#include <cstring>


using namespace std;


/*============================================================================
    Constructor
 ============================================================================ */
OCS_IPN_CpRelatedSwManagerOI::OCS_IPN_CpRelatedSwManagerOI(string szImpName) : acs_apgcc_objectimplementerinterface_V3(szImpName)
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_CpRelatedSwManagerOI::OCS_IPN_CpRelatedSwManagerOI()", 0));

    m_eventReporter = OCS_IPN_EventReporter::getInstance();

    m_oiHandler = new acs_apgcc_oihandler_V3();
    if(m_oiHandler == 0)
    {
        TRACE((LOG_LEVEL_ERROR, "Memory allocation failed for  acs_apgcc_oihandler_V3",0));
    }

    // Get BladeSwManagment dn
    string dn;
    if(OCS_IPN_Common::getFisrtClassInstance(OCS_IPN_Common::IMM_CLN_BLADESWMANAGEMENT,dn))
    {
        OCS_IPN_Common::IMM_DN_BLADESWMANAGEMENT = dn;
        TRACE((LOG_LEVEL_INFO, "BladeSwMangement DN = %s",0, dn.c_str()));
    }

    string asynActionProgressValue;
    if(OCS_IPN_Common::getStringAttributeValue(OCS_IPN_Common::IMM_DN_BLADESWMANAGEMENT,OCS_IPN_Common::IMM_ATTR_ASYNCACTIONPROGRESS_NAME,asynActionProgressValue))
    {
        OCS_IPN_Common::IMM_DN_BLADESWMASYNCACTIONRESULT = asynActionProgressValue;
        TRACE((LOG_LEVEL_INFO, "asynActionProgress value = %s",0, asynActionProgressValue.c_str()));
    }
}

/*============================================================================
    Destructor
 ============================================================================ */
OCS_IPN_CpRelatedSwManagerOI::~OCS_IPN_CpRelatedSwManagerOI()
{
    newTRACE((LOG_LEVEL_INFO, "Enter OCS_IPN_CpRelatedSwManagerOI::::~OCS_IPN_CpRelatedSwManagerOI()", 0));
    // Delete EventReporter object
    OCS_IPN_EventReporter::deleteInstance(m_eventReporter);

    // Delete OIHandler
    if( m_oiHandler != 0 )
    {
        delete m_oiHandler;
        m_oiHandler = 0;
    }

    TRACE((LOG_LEVEL_INFO, "Exit OCS_IPN_CpRelatedSwManagerOI::::~OCS_IPN_CpRelatedSwManagerOI()", 0));
}

/*============================================================================
    Method: run
 ============================================================================ */
bool OCS_IPN_CpRelatedSwManagerOI::registerOIs()
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_CpRelatedSwManagerOI::registerOIs()", 0));

    // Master file descriptor list or event list
    fd_set masterFDs;
    // Temp file descriptor list
    fd_set readFDs;
    // The maximum FD in the master FD list
    int maxFD;
    // Return value
    int retval;
    // Timeout value
    struct timeval tv;

    // Reset the FD list
    FD_ZERO(&masterFDs);
    FD_ZERO(&readFDs);

    // Add stop event into master FD list
    FD_SET(m_stopEvent.getFd(), &masterFDs);
    TRACE((LOG_LEVEL_INFO, "Stop Event FD: %d", 0, m_stopEvent.getFd()));
    maxFD = m_stopEvent.getFd();

    // Reset to default value before run
    bool running = true;
    m_stopEvent.resetEvent();


    ACS_CC_ReturnType result = ACS_CC_FAILURE;

    while (running && (result != ACS_CC_SUCCESS))
    {
        readFDs = masterFDs;

        // Select with 10 secs timeout
        tv.tv_sec = 10;
        tv.tv_usec = 0;

        retval = select(maxFD + 1, &readFDs, NULL, NULL, &tv);

        if (retval == -1)
        {
            if (errno == EINTR)
            {
                // A signal was caught
                continue;
            }

            TRACE((LOG_LEVEL_ERROR, "select error: %d - %s", 0, errno, strerror(errno)));
            m_eventReporter->reportEvent(36000, "Select failed", "SOFTWARE");

            break;
        }
        // Timeout
        // Register the OIs until successfully or stop.
        if (retval == 0)
        {
            // Register as OI
            TRACE((LOG_LEVEL_INFO, "Add class implementer, class name: %s", 0, OCS_IPN_Common::IMM_CLN_CPRELATEDSWMANAGER.c_str()));

            result = m_oiHandler->addClassImpl(this,OCS_IPN_Common::IMM_CLN_CPRELATEDSWMANAGER.c_str());

            if (result != ACS_CC_SUCCESS)
            {
                ostringstream trace;
                trace << "Failed to add class implementer, class name: "<< OCS_IPN_Common::IMM_CLN_CPRELATEDSWMANAGER.c_str();
                TRACE((LOG_LEVEL_ERROR, trace.str().c_str(), 0));

                // Remove class implementer
                if( m_oiHandler != 0 )
                {
                   m_oiHandler->removeClassImpl(this,OCS_IPN_Common::IMM_CLN_CPRELATEDSWMANAGER.c_str());
                }

            }
            else
            {
                TRACE((LOG_LEVEL_INFO, "Successfully to add class implementer, class name: %s", 0, OCS_IPN_Common::IMM_CLN_CPRELATEDSWMANAGER.c_str()));
            }

        }

        if (FD_ISSET(m_stopEvent.getFd(), &readFDs))
        {
            TRACE((LOG_LEVEL_INFO, "Received stop event", 0));
            m_stopEvent.resetEvent();
            running = false;
        }
    }

    return (result == ACS_CC_SUCCESS);

 }
/*============================================================================
    Method: run
 ============================================================================ */
void OCS_IPN_CpRelatedSwManagerOI::run()
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_CpRelatedSwManagerOI::run()", 0));

    ACS_CC_ReturnType result = ACS_CC_FAILURE;

    //Register OIs
    if(!registerOIs())
        return;

    // Master file descriptor list or event list
    fd_set masterFDs;
    // Temp file descriptor list
    fd_set readFDs;
    // The maximum FD in the master FD list
    int maxFD;
    // Return value
    int retval;
    // Timeout value
    struct timeval tv;

    // Reset the FD list
    FD_ZERO(&masterFDs);
    FD_ZERO(&readFDs);

    // Add OI fd & stop event into master FD list
    FD_SET(this->getSelObj(), &masterFDs);
    FD_SET(m_stopEvent.getFd(), &masterFDs);
    TRACE((LOG_LEVEL_INFO, "OI FD: %d - Stop Event FD: %d", 0, this->getSelObj(), m_stopEvent.getFd()));
    maxFD = max(this->getSelObj(), m_stopEvent.getFd());

    // Reset to default value before run
    bool running = true;
    m_stopEvent.resetEvent();

    // Run the IMM event loop and disptach()
    while (running)
    {
        readFDs = masterFDs;

        // Select with 60 secs timeout
        tv.tv_sec = 60;
        tv.tv_usec = 0;

        retval = select(maxFD + 1, &readFDs, NULL, NULL, &tv);

        if (retval == -1)
        {
            if (errno == EINTR)
            {
                // A signal was caught
                continue;
            }

            TRACE((LOG_LEVEL_ERROR, "select error: %d - %s", 0, errno, strerror(errno)));
            m_eventReporter->reportEvent(36000, "Select failed", "SOFTWARE");

            break;
        }

        if (retval == 0)
        {
            continue;
        }

        if (FD_ISSET(m_stopEvent.getFd(), &readFDs))
        {
            TRACE((LOG_LEVEL_INFO, "Received stop event", 0));
            m_stopEvent.resetEvent();
            running = false;
        }

        if (FD_ISSET(this->getSelObj(), &readFDs))
        {
            result = this->dispatch(ACS_APGCC_DISPATCH_ONE);

            if (result != ACS_CC_SUCCESS)
            {
                TRACE((LOG_LEVEL_ERROR, "Failed to dispatch IMM event", 0));
                m_eventReporter->reportEvent(36000, "Failed to dispatch IMM event", "SOFTWARE");
                running = false;
            }
        }
    }

    running = false;

    // Remove class implementer
    if( m_oiHandler != 0 )
    {
        m_oiHandler->removeClassImpl(this,OCS_IPN_Common::IMM_CLN_CPRELATEDSWMANAGER.c_str());
    }

    TRACE((LOG_LEVEL_INFO, "OI stopped gracefully", 0));
}

/*============================================================================
    Method: stop
 ============================================================================ */
void OCS_IPN_CpRelatedSwManagerOI::stop()
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_CpRelatedSwManagerOI::stop()", 0));

    m_stopEvent.setEvent();
}

/*============================================================================
    Method: create
 ============================================================================ */
ACS_CC_ReturnType OCS_IPN_CpRelatedSwManagerOI::create(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId, const char *className, const char* parentName, ACS_APGCC_AttrValues **)
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_CpRelatedSwManagerOI::create(%lu, %lu, %s, %s, attr)", 0, oiHandle, ccbId, ((className) ? className : "NULL"), ((parentName) ? parentName : "NULL")));


    ACS_CC_ReturnType result = ACS_CC_SUCCESS;

    // Do nothing

    return result;
}

/*============================================================================
    Method: deleted
 ============================================================================ */
ACS_CC_ReturnType OCS_IPN_CpRelatedSwManagerOI::deleted(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId, const char *objName)
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_CpRelatedSwManagerOI::deleted(%lu, %lu, %s)", 0, oiHandle, ccbId, ((objName) ? objName : "NULL")));

    ACS_CC_ReturnType result = ACS_CC_FAILURE;

    // Do nothing and object must not be deleted

    return result;
}

/*============================================================================
    Method: modify
 ============================================================================ */
ACS_CC_ReturnType OCS_IPN_CpRelatedSwManagerOI::modify(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId, const char *objName, ACS_APGCC_AttrModification** /*attrMods*/)
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_CpRelatedSwManagerOI::modify(%lu, %lu, %s, attrMods)", 0, oiHandle, ccbId, ((objName) ? objName : "NULL")));

    ACS_CC_ReturnType result = ACS_CC_SUCCESS;
    return result;
}

/*============================================================================
    Method: complete
 ============================================================================ */
ACS_CC_ReturnType OCS_IPN_CpRelatedSwManagerOI::complete(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId)
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_CpRelatedSwManagerOI::complete(%lu, %lu)", 0, oiHandle, ccbId));

    ACS_CC_ReturnType result = ACS_CC_SUCCESS;
    return result;
}

/*============================================================================
    Method: abort
 ============================================================================ */
void OCS_IPN_CpRelatedSwManagerOI::abort(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId)
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_CpRelatedSwManagerOI::abort(%lu, %lu)", 0, oiHandle, ccbId));

    // Reset update info

}

/*============================================================================
Method: apply
============================================================================ */
void OCS_IPN_CpRelatedSwManagerOI::apply(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_CcbId ccbId)
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_CpRelatedSwManagerOI::apply(%lu, %lu)", 0, oiHandle, ccbId));

}

/*============================================================================
Method: updateRuntime
============================================================================ */
ACS_CC_ReturnType OCS_IPN_CpRelatedSwManagerOI::updateRuntime(const char* p_objName, const char** )
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_CpRelatedSwManagerOI::updateRuntime(%s)", 0, ((p_objName) ? p_objName : "NULL")));

    ACS_CC_ReturnType result = ACS_CC_SUCCESS;

    // Do nothing

    return result;
}

/*============================================================================
    Method: adminOperationCallback
============================================================================ */
void OCS_IPN_CpRelatedSwManagerOI::adminOperationCallback(ACS_APGCC_OiHandle oiHandle, ACS_APGCC_InvocationType invocation, const char* p_objName, ACS_APGCC_AdminOperationIdType operationId, ACS_APGCC_AdminOperationParamType** paramList)
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_CpRelatedSwManagerOI::adminOperationCallback(%lu, %lu, %s, %lu, paramList)", 0, oiHandle, invocation, ((p_objName) ? p_objName : "NULL"), operationId));

    SaAisErrorT resultOfOperation = SA_AIS_OK;

    switch(operationId)
    {
        case OCS_IPN_Common::INSTALL_IPNA_FILES:
        {
            //1. Get parameters
            int32_t fbn = 0;
            std::string fileName =""; // Name of zip package
            int i = 0;

            while(paramList[i] != 0)
            {
                std::string myOtName(paramList[i]->attrName);  // name

                if (strcmp(myOtName.c_str(),"fileName") == 0)
                {
                    std::string myOtValue((char*)paramList[i]->attrValues);  // value
                    fileName = myOtValue;
                    TRACE((LOG_LEVEL_INFO, "parameter name = %s, value = %s",0, myOtName.c_str(), myOtValue.c_str()));
                }
                if (strcmp(myOtName.c_str(),"fbn") == 0)
                {
                    fbn = *(reinterpret_cast<int32_t *> (paramList[i]->attrValues));  // value
                    TRACE((LOG_LEVEL_INFO, "parameter name = %s,value = %d",0,myOtName.c_str(), fbn));
                }
                else
                {
                    TRACE((LOG_LEVEL_INFO, "Unknown parameter name = %s",0,myOtName.c_str()));
                }
                i++;
            } // END of while loop

            OCS_IPN_Common::ImportResultStateType resultStateType = OCS_IPN_Common::SUCCESS;

            // extract IPNA files to boot folder location from zip file.
            if(!OCS_IPN_Common::loadIpna(fileName))
            {
                resultStateType = OCS_IPN_Common::FAILURE;

                setExitCode( 1, "Failed to extract package");
                resultOfOperation = SA_AIS_ERR_FAILED_OPERATION;
            }

            // update updateAsyncActionProgress
            time_t rawtime;
            struct tm * timeinfo;

            time ( &rawtime );
            timeinfo = localtime ( &rawtime );
            char timeStr[30];
            memset (timeStr,0,30);
            sprintf(timeStr,"%d-%d-%d %d:%d:%d",timeinfo->tm_year + 1900,timeinfo->tm_mon + 1,timeinfo->tm_mday,timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec);
            string timeStr1(timeStr);

            updateAsyncActionProgress(resultStateType, OCS_IPN_Common::EXECUTED, OCS_IPN_Common::OCS_IPN_Common::FINISHED,timeStr1);

            break;
        }
        default:
        {
            TRACE((LOG_LEVEL_INFO, "Default. Not match to any action",0));
            break;
        }
    }

    ACS_CC_ReturnType rc = adminOperationResult(oiHandle, invocation, resultOfOperation);
    if(rc != ACS_CC_SUCCESS)
    {
        TRACE((LOG_LEVEL_ERROR, "Failure occurred in sending AdminOperation Result",0));
        return;
    }

    // Do nothing
}

/*============================================================================
    Method: modifyIntAttr
============================================================================ */
bool OCS_IPN_CpRelatedSwManagerOI::modifyIntAttr(OmHandler& omHandler, const std::string& objectName, const std::string& attrName,int value, const std::string& transName)
{
    ACS_CC_ReturnType returnCode;

    // Modify
    ACS_CC_ImmParameter attributeRDN1;

    /*Fill the rdn Attribute */
    attributeRDN1.attrName = (char*)attrName.c_str();
    attributeRDN1.attrType = ATTR_INT32T;
    attributeRDN1.attrValuesNum = 1;
    void* valueRDN1[1]={reinterpret_cast<void*>(&value)};
    attributeRDN1.attrValues = valueRDN1;

    returnCode = omHandler.modifyAttribute(objectName.c_str(), &attributeRDN1, transName);

    if(returnCode == ACS_CC_SUCCESS)
    {
        //TRACE((LOG_LEVEL_INFO, "Modify Attribute %s completed\n", 0, attrName.c_str()));
    }
    else
    {
        newTRACE((LOG_LEVEL_ERROR, "error - Modify attribute %s failed with error: %d - %s\n", 0,
                attrName.c_str(),
                omHandler.getInternalLastError(), omHandler.getInternalLastErrorText()));
     }

    return (returnCode == ACS_CC_SUCCESS);
}

/*============================================================================
    Method: modifyStringAttr
============================================================================ */
bool OCS_IPN_CpRelatedSwManagerOI::modifyStringAttr(OmHandler& omHandler, const std::string& objectName, const std::string& attrName,const std::string& value, const std::string& transName)
{
    ACS_CC_ReturnType returnCode;

    // Modify
    ACS_CC_ImmParameter attributeRDN1;

    /*Fill the rdn Attribute */
    attributeRDN1.attrName = (char*)attrName.c_str();
    attributeRDN1.attrType = ATTR_STRINGT;
    attributeRDN1.attrValuesNum = 1;
    void* valueRDN1[1]={reinterpret_cast<void*>((char*)value.c_str())};
    attributeRDN1.attrValues = valueRDN1;

    returnCode = omHandler.modifyAttribute(objectName.c_str(), &attributeRDN1, transName);

    if(returnCode == ACS_CC_SUCCESS)
    {
        //TRACE((LOG_LEVEL_INFO, "Modify Attribute %s completed with transaction %s \n", 0,
        //        attrName.c_str(), transName.c_str()));
    }
    else
    {
        newTRACE((LOG_LEVEL_ERROR, "error - Modify attribute %s failed with error: %d - %s\n", 0,
                attrName.c_str(),
                omHandler.getInternalLastError(), omHandler.getInternalLastErrorText()));
    }

    return (returnCode == ACS_CC_SUCCESS);

}

/*============================================================================
    Method: updateAsyncActionProgress
============================================================================ */
bool OCS_IPN_CpRelatedSwManagerOI::updateAsyncActionProgress(OCS_IPN_Common::ImportResultStateType importResultState, OCS_IPN_Common::ImportResultInfoType importResultInfo, OCS_IPN_Common::ActionStateTypeType actionStateType, string time)
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_CpRelatedSwManagerOI::updateAsyncActionProgress", 0));

    ACS_CC_ReturnType errorCode = ACS_CC_SUCCESS;

    OmHandler omHandler;
    if (omHandler.Init() == ACS_CC_FAILURE)
    {
       TRACE((LOG_LEVEL_ERROR, "omHandler Init() failed",0));
       return false;
    }

    string transName = "updateAsyncActionProgress";

    // Update importResultState
    modifyIntAttr(omHandler, OCS_IPN_Common::IMM_DN_BLADESWMASYNCACTIONRESULT, OCS_IPN_Common::IMM_ATTR_BLADESWMASYNCACTIONRESULT_RESULT_NAME,importResultState,transName);

    // Update importResultInfo
    modifyIntAttr(omHandler, OCS_IPN_Common::IMM_DN_BLADESWMASYNCACTIONRESULT, OCS_IPN_Common::IMM_ATTR_BLADESWMASYNCACTIONRESULT_RESULTINFO_NAME,importResultInfo,transName);

    // Update actionStateType
    modifyIntAttr(omHandler, OCS_IPN_Common::IMM_DN_BLADESWMASYNCACTIONRESULT, OCS_IPN_Common::IMM_ATTR_BLADESWMASYNCACTIONRESULT_STATE_NAME,actionStateType,transName);

    // Update time
    modifyStringAttr(omHandler, OCS_IPN_Common::IMM_DN_BLADESWMASYNCACTIONRESULT, OCS_IPN_Common::IMM_ATTR_BLADESWMASYNCACTIONRESULT_TIMEACTIONCOMPLETED_NAME,time,transName);

    errorCode = omHandler.applyRequest(transName);

    if(errorCode == ACS_CC_SUCCESS)
    {
        //TRACE((LOG_LEVEL_INFO, "Apply completed\n", 0));
    }
    else
    {
        TRACE((LOG_LEVEL_ERROR, "Apply failed with error: %d - %s\n", 0, omHandler.getInternalLastError(), omHandler.getInternalLastErrorText()));
        // Reset all requests with the transaction name
        omHandler.resetRequest(transName);
    }

    // Finalize OM
    omHandler.Finalize();

    return (errorCode == ACS_CC_SUCCESS);
}

