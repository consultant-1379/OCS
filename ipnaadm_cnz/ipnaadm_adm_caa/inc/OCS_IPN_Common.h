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
// OCS_IPN_Common.h
//
// DESCRIPTION
// Class declaration for class OCS_IPN_Common
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


#ifndef OCS_IPN_COMMOM_H_
#define OCS_IPN_COMMOM_H_


#include <string>
#include <vector>
#include <map>


class OCS_IPN_Common
{
public:

    static std::string SFTP_ROOT_PATH;
    static std::string IPNA_NBI_PATH;

    static std::string IMM_CPRELATEDMANAGER_IMPL_NAME;

    /***********************************************************
             BladeSwManagement class
     ***********************************************************/
    static std::string IMM_CLN_BLADESWMANAGEMENT;
    static std::string IMM_DN_BLADESWMANAGEMENT;
    static std::string IMM_ATTR_ASYNCACTIONPROGRESS_NAME;


    /***********************************************************
             CpRelatedSwManager class
     ***********************************************************/
    static std::string IMM_CLN_CPRELATEDSWMANAGER;


    /***********************************************************
             BladeSwMAsyncActionResult struct attributes
     ***********************************************************/
    static std::string IMM_DN_BLADESWMASYNCACTIONRESULT;
    static std::string IMM_ATTR_BLADESWMASYNCACTIONRESULT_RESULT_NAME;
    static std::string IMM_ATTR_BLADESWMASYNCACTIONRESULT_RESULTINFO_NAME;
    static std::string IMM_ATTR_BLADESWMASYNCACTIONRESULT_STATE_NAME;
    static std::string IMM_ATTR_BLADESWMASYNCACTIONRESULT_TIMEACTIONCOMPLETED_NAME;

    enum IPNA_ACTIONS
    {
        INSTALL_IPNA_FILES = 1
    };

    typedef enum
    {
        SUCCESS         = 0,
        FAILURE         = 1,
        NOT_AVAILABLE   = 2
    } ImportResultStateType;

    typedef enum
    {
        EXECUTED                        = 0,
        INTERNAL_ERROR                  = 1,
        INVALID_PACKAGE                 = 4,
        PACKAGE_ALREADY_INSTALLED       = 5,
        MAXIMUM_LOAD_MODULES_INSTALLED  = 6,
        DISK_QUOTA_EXCEEDED             = 7
    } ImportResultInfoType;

    typedef enum
    {
        RUNNING = 0,
        FINISHED =1
    } ActionStateTypeType;

    /****************************************************************************
     * Method:  loadIpna
     * Description: Unzip ipna load module package from /data/opt/ap/internal_root/sw_package/CP to sftp root path (/data/apz/data)
     * Return: true if copy successfully, false if not.
     *****************************************************************************
     */
    static bool loadIpna(const std::string ipnaLoadModules);

    /****************************************************************************
     * Method:  getFisrtClassInstance()
     * Description: get DN of first instance of a class
     * Param [in]: className - class name to get first instance
     * Param [out]: dn - DN of first instance of className
     * Return: true if successful, false otherwise
     *****************************************************************************
     */
    static bool getFisrtClassInstance(const std::string className, std::string& dn);

    /***************************************************************************
     * Method:  getStringAttributeValue()
     * Description: Get
     *****************************************************************************
     */
    static bool getStringAttributeValue(const std::string dn, const std::string attribute, std::string& value);

};
#endif /* OCS_IPN_COMMOM_H_ */
