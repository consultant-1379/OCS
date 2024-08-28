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
// Methods implementation for class OCS_IPN_Common
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


#include "OCS_IPN_Common.h"
#include "OCS_IPN_Trace.h"
#include "acs_apgcc_omhandler.h"

#include <sstream>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>



using namespace std;

string OCS_IPN_Common::SFTP_ROOT_PATH                      = "/data/apz/data/";
string OCS_IPN_Common::IPNA_NBI_PATH                       = "/data/opt/ap/internal_root/sw_package/CP/";
string OCS_IPN_Common::IMM_CPRELATEDMANAGER_IMPL_NAME      = "CpRelatedSwManagerImplementer";

/***********************************************************
         BladeSwManagement class
 ***********************************************************/
string OCS_IPN_Common::IMM_CLN_BLADESWMANAGEMENT            = "BladeSwManagementBladeSwM";
string OCS_IPN_Common::IMM_DN_BLADESWMANAGEMENT             = "BladeSwManagementbladeSwMId=1";
string OCS_IPN_Common::IMM_ATTR_ASYNCACTIONPROGRESS_NAME    = "asyncActionProgress";

/***********************************************************
         CpRelatedSwManager class
 ***********************************************************/
string OCS_IPN_Common::IMM_CLN_CPRELATEDSWMANAGER            = "BladeSwManagementCpRelatedSwManager";

/***********************************************************
     BladeSwMAsyncActionResult struct attributes
 ***********************************************************/
string OCS_IPN_Common::IMM_DN_BLADESWMASYNCACTIONRESULT                              = "id=bladeSwmResultInstance,BladeSwManagementBladeSwMId=1";
string OCS_IPN_Common::IMM_ATTR_BLADESWMASYNCACTIONRESULT_RESULT_NAME                = "result";
string OCS_IPN_Common::IMM_ATTR_BLADESWMASYNCACTIONRESULT_RESULTINFO_NAME            = "resultInfo";
string OCS_IPN_Common::IMM_ATTR_BLADESWMASYNCACTIONRESULT_STATE_NAME                 = "state";
string OCS_IPN_Common::IMM_ATTR_BLADESWMASYNCACTIONRESULT_TIMEACTIONCOMPLETED_NAME   = "timeActionCompleted";

/****************************************************************************
 * Method:  loadIpna
 *****************************************************************************
 */
bool OCS_IPN_Common::loadIpna(const string ipnaLoadModules)
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_Common::loadIpna()", 0));

    // Check if zip file exist.
    string fullPath = ipnaLoadModules;
    FILE * pFile;
    pFile = fopen (fullPath.c_str(),"r");
    if (pFile == NULL)
    {
        TRACE((LOG_LEVEL_ERROR, "File %s does not exist", 0, fullPath.c_str()));
        return false;
    }

    fclose(pFile);

    ostringstream trace;
    trace << "unzip -o " << fullPath << " -d " << OCS_IPN_Common::SFTP_ROOT_PATH << "> /dev/null 2>&1";
    int returnCode = system(trace.str().c_str());
    if( returnCode == 0)
    {
        TRACE((LOG_LEVEL_INFO, "unzip %s successfully",0, ipnaLoadModules.c_str()));
        return true;
    }
    else
    {
        TRACE((LOG_LEVEL_ERROR, "system function failed with error: %s",0, strerror(errno)));
        return false;
    }

    return true;
}

/****************************************************************************
 * Method:  getFisrtClassInstance()
 *****************************************************************************
 */
bool OCS_IPN_Common::getFisrtClassInstance(const string className, string& dn)
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_Common::getFisrtClassInstance", 0));

    OmHandler omHandler;
    ACS_CC_ReturnType ret = ACS_CC_SUCCESS;
    std::vector<std::string> dnList;

    try
    {
        ret = omHandler.Init();
        if (ret != ACS_CC_SUCCESS)
        {
           TRACE((LOG_LEVEL_ERROR, "Init OMHandler failed: %d - %s", 0, omHandler.getInternalLastError(), omHandler.getInternalLastErrorText()));
           throw 1;
        }

        ret = omHandler.getClassInstances(className.c_str(), dnList);

        if (ret != ACS_CC_SUCCESS)
        {
           TRACE((LOG_LEVEL_ERROR, "Get class instance failed: %d - %s", 0, omHandler.getInternalLastError(), omHandler.getInternalLastErrorText()));
           throw 1;
        }

        dn = dnList[0];
    }
    catch(...)
    {
        omHandler.Finalize();
        return false;
    }

    omHandler.Finalize();
    return (ret == ACS_CC_SUCCESS);
}


/***************************************************************************
 * Method:  getStringAttributeValue()
 *****************************************************************************
 */
bool OCS_IPN_Common::getStringAttributeValue(const string dn, const string attribute, string& value)
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_Common::getNameAttributeValue", 0));

    OmHandler omHandler;
    ACS_CC_ReturnType ret = ACS_CC_SUCCESS;
    ACS_CC_ImmParameter paramToFind;
    paramToFind.attrName = NULL;
    char* pszAttrValue;

    try
    {

        ret = omHandler.Init();
        if (ret != ACS_CC_SUCCESS)
        {
           TRACE((LOG_LEVEL_ERROR, "Init OMHandler failed: %d - %s", 0, omHandler.getInternalLastError(), omHandler.getInternalLastErrorText()));
           throw 1;
        }

        paramToFind.attrName = (char*)malloc(attribute.length() + 1);

        if(paramToFind.attrName == NULL)
        {
            TRACE((LOG_LEVEL_ERROR, "Failed to allocate memory for paramToFind.attrName",0));
            throw 1;
        }

        strcpy(paramToFind.attrName, attribute.c_str());
        TRACE((LOG_LEVEL_INFO, "paramToFind.attrName: %s", 0, paramToFind.attrName));

        ret = omHandler.getAttribute(dn.c_str(),&paramToFind);

        if (ret != ACS_CC_SUCCESS)
        {
           TRACE((LOG_LEVEL_ERROR, "Get attribute failed: %d - %s", 0, omHandler.getInternalLastError(), omHandler.getInternalLastErrorText()));
           throw 1;
        }

        if(paramToFind.attrValuesNum == 0)
        {
        	TRACE((LOG_LEVEL_ERROR, "Get attribute failed, attribute values number is 0: %d - %s", 0, omHandler.getInternalLastError(), omHandler.getInternalLastErrorText()));
        	throw 1;
        }
        pszAttrValue = (reinterpret_cast<char*>(*(paramToFind.attrValues)));
        value.clear();
        value.append(pszAttrValue);
    }
    catch (...)
    {
        free(paramToFind.attrName);
        omHandler.Finalize();
        return false;
    }

    free(paramToFind.attrName);
    omHandler.Finalize();
    return (ret == ACS_CC_SUCCESS);
}
