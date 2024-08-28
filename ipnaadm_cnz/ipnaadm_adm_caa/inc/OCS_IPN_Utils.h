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
// OCS_IPN_Global.h
//
// DESCRIPTION 
// This file contains the utility functions.
//
// DOCUMENT NO
// 190 89-CAA 109 1405
//
// AUTHOR
// XDT/DEK XTUANGU
//
//******************************************************************************
// *** Revision history ***
// 2011-09-07 Created by XTUANGU
//******************************************************************************

#ifndef  OCS_IPN_Utils_h
#define  OCS_IPN_Utils_h

#include "stdint.h"
#include <string>
#include <vector>

const int NO_OF_AP=16;
const std::string APNAMES[NO_OF_AP][2]=
{
    {"AP1A","AP1B"},
    {"AP2A","AP2B"},
    {"AP3A","AP3B"},
    {"AP4A","AP4B"},
    {"AP5A","AP5B"},
    {"AP6A","AP6B"},
    {"AP7A","AP7B"},
    {"AP8A","AP8B"},
    {"AP9A","AP9B"},
    {"AP10A","AP10B"},
    {"AP11A","AP11B"},
    {"AP12A","AP12B"},
    {"AP13A","AP13B"},
    {"AP14A","AP14B"},
    {"AP15A","AP15B"},
    {"AP16A","AP16B"}
};

// The AP IP-addresses are fixed according to the tables below.
const std::string APIP[NO_OF_AP][2][2]=
{
    {{"192.168.169.1","192.168.170.1"},{"192.168.169.2","192.168.170.2"}},
    {{"192.168.169.3","192.168.170.3"},{"192.168.169.4","192.168.170.4"}},
    {{"192.168.169.5","192.168.170.5"},{"192.168.169.6","192.168.170.6"}},
    {{"192.168.169.7","192.168.170.7"},{"192.168.169.8","192.168.170.8"}},
    {{"192.168.169.9","192.168.170.9"},{"192.168.169.10","192.168.170.10"}},
    {{"192.168.169.11","192.168.170.11"},{"192.168.169.12","192.168.170.12"}},
    {{"192.168.169.13","192.168.170.13"},{"192.168.169.14","192.168.170.14"}},
    {{"192.168.169.15","192.168.170.15"},{"192.168.169.16","192.168.170.16"}},
    {{"192.168.169.17","192.168.170.17"},{"192.168.169.18","192.168.170.18"}},
    {{"192.168.169.19","192.168.170.19"},{"192.168.169.20","192.168.170.20"}},
    {{"192.168.169.21","192.168.170.21"},{"192.168.169.22","192.168.170.22"}},
    {{"192.168.169.23","192.168.170.23"},{"192.168.169.24","192.168.170.24"}},
    {{"192.168.169.25","192.168.170.25"},{"192.168.169.26","192.168.170.26"}},
    {{"192.168.169.27","192.168.170.27"},{"192.168.169.28","192.168.170.28"}},
    {{"192.168.169.29","192.168.170.29"},{"192.168.169.30","192.168.170.30"}},
    {{"192.168.169.31","192.168.170.31"},{"192.168.169.32","192.168.170.32"}}
};



class OCS_IPN_Utils
{

public:

	/****************************************************************************
	* Method:	putBuffer
	* Description: append a buffer to another buffer
	* Param [in]: buf
	* Param [in]: bootcontents_out
	* Param [out]: N/A
	* Return: N/A
	*****************************************************************************
	*/
	static void putBuffer(char* buf, uint32_t len, char* bootcontents_out);

	/****************************************************************************
	* Method:	toUpper
	* Description: change string to upper case
	* Param [in]: str - string buffer
	* Param [out]: N/A
	* Return: N/A
	*****************************************************************************
	*/
	static void toUpper(std::string& str);


	/****************************************************************************
	* Method:	ulongToDotIP
	* Description: change ip from number to dotted string
	* Param [in]: ip - ip address in number
	* Param [out]: N/A
	* Return: ip address in dotted string
	*****************************************************************************
	*/
	static std::string ulongToDotIP(uint32_t ip);


	/****************************************************************************
	* Method:	isClassicCP
	* Description: check if the APZ type is classic or not
	* Param [in]: N/A
	* Param [out]: N/A
	* Return: true if classic or false otherwise
	*****************************************************************************
	*/
	static bool isClassicCP();

	/******************************************************************************
	*Method:   isActiveNode
    * Description: check if the AP node is active or not
    * Param [in]: N/A
    * Param [out]: N/A
    * Return: true if AP active node or false otherwise
    *****************************************************************************
    */
    static bool isActiveNode();

   /****************************************************************************
    * Method:   getConfigurationFiles
    * Description: Get *.ipn* from a directory
    * Param [in]: N/A
    * Param [out]: N/A
    * Return: 0 if successful or errno otherwise
    *****************************************************************************
    */
    static int getConfigurationFiles(std::vector<std::string> &configurationFiles, const std::string directory);

   /****************************************************************************
    * Method:   copyBootFile
    * Description: Copy boot file from its template
    * Param [in]: srcFile - boot file template (boot.ipnX.cp_loading or boot.ipnX.not_loading)
    * Param [in]: dstFile - boot file to be copied
    * Param [in]: overwrite - override or not in copy operation
    * Param [out]: N/A
    * Return: 0 if successful or Linux errno otherwise
    *****************************************************************************
    */
    static int copyBootFile(const char* srcFile, const char* dstFile);

    /****************************************************************************
     * Method:  isConfigurationFile
     * Description: check if the fileName is *.ipn* format
     * Param [in]: fileName - file name to check
     * Return: true if correct, false otherwise
     ****************************************************************************/
    static bool isConfigurationFile(const char* fileName);

    /****************************************************************************
     * Method:  fileExists
     * Description: check if the fileName is existed
     * Param [in]: fileName - file name to check
     * Return: true if exists, false otherwise
     ****************************************************************************/
    static bool fileExists(const char* filename);


    /****************************************************************************
     * Method:  strICompare
     * Description: Compare two string without case sensitive
     * Param [in]: str1 - first string
     * Param [in]: str2 - second string
     * Return: 0 if equal, 1 if str1 > str2, -1 if str 1 < str2
     ****************************************************************************/
    static int strICompare(const char* str1, const char* str2);


    /****************************************************************************
     * Method:  getThisNode
     * Description: get IP addresses of this AP node
     * Param [out]: ipdot_vec - dotted ip addresses
     * Return: true if success, false otherwise
     ****************************************************************************/

    static bool getThisNode(std::vector<std::string> &ipdot_vec);

private:
	OCS_IPN_Utils();

};
#endif
