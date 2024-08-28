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
// OCS_IPN_Global.cpp
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
//******************************************************************************

// 2011-09-07 Created by XTUANGU

#include "OCS_IPN_Utils.h"
#include "OCS_IPN_Trace.h"
#include "acs_prc_api.h"
#include "acs_apgcc_paramhandling.h"
#include "acs_apgcc_omhandler.h"
#include "ACS_DSD_Server.h"
#include <vector>
#include <iostream>
#include <sstream>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/sendfile.h>
#include <fstream>



using namespace std;

/****************************************************************************
 * Method:	putBuffer
 * Description: append a buffer to another buffer
 * Param [in]: buf
 * Param [in]: bootcontents_out
 * Param [out]: N/A
 * Return: N/A
 *****************************************************************************
 */
void OCS_IPN_Utils::putBuffer(char* buf, uint32_t len, char* bootcontents_out)
{
	uint32_t totlen;

	totlen = strlen(bootcontents_out);
	strncpy(bootcontents_out+totlen, buf, len);
	bootcontents_out[totlen+len] = 0;
}

/****************************************************************************
 * Method:	toUpper
 * Description: change string to upper case
 * Param [in]: str - string buffer
 * Param [out]: N/A
 * Return: N/A
 *****************************************************************************
 */
void OCS_IPN_Utils::toUpper(string& str)
{
    string::iterator it;

    for (it = str.begin(); it != str.end(); ++it)
    {
        (*it) = (char) ::toupper(*it);
    }
}

/****************************************************************************
 * Method:	ulongToDotIP
 * Description: change ip from number to dotted string
 * Param [in]: ip - ip address in number
 * Param [out]: N/A
 * Return: ip address in dotted string
 *****************************************************************************
 */
string OCS_IPN_Utils::ulongToDotIP(uint32_t ip)
{
	ostringstream ss;
	ss<<(ip&0x000000FF)<<"."<<((ip&0x0000FF00)>>8)<<"."<<((ip&0x00FF0000)>>16)
		<<"."<<((ip&0xFF000000)>>24);
	string dotIp=ss.str();
	return dotIp;
}


/*****************************************************************************
 *  Parameter   cpAndProtocolType (old name: ACS_APCONFBIN_CpAndProtocolType)
 *  Format  unsignedInt
 *  Contains information about the CP type the AP is connected to and what
 *  communication protocol to use on the processor test bus when communicating
 *  with the CP
 *
 *           Value   CP type       Protocol
 *           -----   -------       --------
 *           0       APZ 212 3x    SDLC
 *           1       APZ 212 3x    TCP/IP
 *           2       APZ 212 40    TCP/IP
 *           3       APZ 212 50    TCP/IP
 *           4       APZ 212 55    TCP/IP
 *           4       MSC-S BC SPX  TCP/IP
 *****************************************************************************/
bool OCS_IPN_Utils::isClassicCP()
{
	newTRACE((LOG_LEVEL_INFO, "OCS_IPN_Utils::isClassicCP()",0));

    static bool isClassicSys = false;

    // Get the AxeFunctions DN
    OmHandler immHandle;
    ACS_CC_ReturnType ret;
    std::vector<std::string> dnList;

    ret = immHandle.Init();
    if (ret != ACS_CC_SUCCESS)
    {
        TRACE((LOG_LEVEL_ERROR, "Init OMHandler failed: %d - %s", 0,
                immHandle.getInternalLastError(),
                immHandle.getInternalLastErrorText()));

        return isClassicSys;
    }

    ret = immHandle.getClassInstances("AxeFunctions", dnList);

    if (ret != ACS_CC_SUCCESS)
    {
        TRACE((LOG_LEVEL_ERROR, "Get class instance failed: %d - %s", 0,
               immHandle.getInternalLastError(),
               immHandle.getInternalLastErrorText()));

        immHandle.Finalize();
        return isClassicSys;
    }
    else
    {
        immHandle.Finalize();
        TRACE((LOG_LEVEL_INFO, "AxeFunctions DN: %s", 0, dnList[0].c_str()));
    }

    acs_apgcc_paramhandling phaObject;
    const char *pAttrName = "apzProtocolType";
    int apzProtocolTypeValue = 0;

    ret = phaObject.getParameter(dnList[0], pAttrName, &apzProtocolTypeValue);

    if (ret == ACS_CC_SUCCESS)
    {
        if ((apzProtocolTypeValue == 0) || (apzProtocolTypeValue == 1))
        {
            isClassicSys = true;
        }
    }
    else
    {
        TRACE((LOG_LEVEL_ERROR, "Config::isClassicCP(): Reading PHA param apzProtocolType failed", 0));
        return isClassicSys;
    }

    return isClassicSys;

}


/****************************************************************************
 * Method:  isActiveNode
 * ***************************************************************************
 */
bool OCS_IPN_Utils::isActiveNode()
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_Utils::isActiveNode()",0));

    ACS_PRC_API prcApi;
    int nodeState = prcApi.askForNodeState();

    // nodeState = -1 Error detected, 1 Active, 2 Passive.
    TRACE((LOG_LEVEL_INFO, "This AP Node state (-1 Error detected, 1 Active, 2 Passive): %d",0, nodeState));
   return (nodeState == 1);
}


/****************************************************************************
 * Method:  configurationFiles
 *****************************************************************************
 */
int OCS_IPN_Utils::getConfigurationFiles(vector<string> &configurationFiles, const string directory)
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_Utils::getConfigurationFiles", 0));

    DIR *dir;
    struct dirent *dirEnt;
    struct stat st;
    char            ffile[80];

    //1. Find the *.ipn* files
    strcpy(ffile, directory.c_str());
    dir = opendir(ffile);
    if(dir == NULL)
    {
        TRACE((LOG_LEVEL_ERROR, "opendir failed:", 0,  strerror(errno)));
        return errno;
    }
    else
    {
        while ((dirEnt = readdir(dir))!= NULL)
        {
            const string currentFile = dirEnt->d_name;
            const string fullcurrentFile = directory + currentFile;

            if (stat(fullcurrentFile.c_str(), &st) == -1)
               break;

            const bool isDirectory = (st.st_mode & S_IFDIR) != 0;

            if (isDirectory)
               continue;

            if(isConfigurationFile(dirEnt->d_name))
            {
                TRACE((LOG_LEVEL_INFO, "boot file: %s", 0, fullcurrentFile.c_str()));
                configurationFiles.push_back(currentFile);
            }
        }

        closedir(dir);
    }

    return 0;
}

/****************************************************************************
 * Method:  copyBootFile
 ****************************************************************************/
int OCS_IPN_Utils::copyBootFile(const char* srcFile , const char* dstFile)
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_Common::copyBootFile()", 0));

    int read_fd;
    int write_fd;
    struct stat stat_buf;
    int retCode;

    off_t offset = 0;

    /* Open the input file. */
    if ((read_fd = open(srcFile, O_RDONLY)) < 0)
    {
        retCode = errno;

        TRACE((LOG_LEVEL_ERROR, "Open file %s failed with error: %s ",0,srcFile, strerror(errno)));
        return retCode;
    }
    /* Stat the input file to obtain its size. */
    if (fstat(read_fd, &stat_buf) < 0)
    {
        retCode = errno;

        TRACE((LOG_LEVEL_ERROR, "Read file information failed with error: %s",0,strerror(errno)));
        close(read_fd);
        return retCode;
    }

    /* Remove destination file if it exists */
    if(OCS_IPN_Utils::fileExists(dstFile))
    {
        if(remove(dstFile) != 0)
        {

            retCode = errno;

            TRACE((LOG_LEVEL_ERROR, "Delete file failed with error: %s",0,strerror(errno)));
            close(read_fd);
            return retCode;
        }
    }

    /* Open the output file for writing, with the same permissions as the
     source file. */
    if ((write_fd = open(dstFile, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0)
    {
        retCode = errno;

        TRACE((LOG_LEVEL_ERROR, "Open file %s failed with error: %s",0,dstFile, strerror(errno)));

        close(read_fd);
        return retCode;
    }

    /* Blast the bytes from one file to the other. */
    if(sendfile(write_fd, read_fd, &offset, stat_buf.st_size) < 0)
    {
        retCode = errno;

        TRACE((LOG_LEVEL_ERROR, "sendfile failed with error: %s",0,strerror(errno)));

        /* Close up. */
        close(read_fd);
        close(write_fd);

        return retCode;
    }

    /* Close up. */
    close(read_fd);
    close(write_fd);

    return 0;
}


/****************************************************************************
 * Method:  isConfigurationFile
 ****************************************************************************/
bool OCS_IPN_Utils::isConfigurationFile(const char* fileName)
{
    char*   p1 = NULL;
    p1 = strchr((char*)fileName, 46);              // Dot
    if (p1 != NULL && p1 != fileName)
    {
        return strncmp(++p1, "ipn", 3) == 0;
    }

    return false;
}


/****************************************************************************
 * Method:  fileExists
 ****************************************************************************/
bool OCS_IPN_Utils::fileExists(const char* filename)
{
    ifstream ifile(filename, ios_base::in);
    bool exist = ifile.good();
    if (exist)
        ifile.close();
    return exist;
}

/****************************************************************************
 * Method:  strICompare
 ****************************************************************************/
int OCS_IPN_Utils::strICompare(const char* str1, const char* str2)
{
    string string1 = string(str1);
    string string2 = string(str2);

    toUpper(string1);
    toUpper(string2);

    return strcmp(string1.c_str(), string2.c_str());
}

/****************************************************************************
 * Method:  getThisNode()
 ****************************************************************************/
bool OCS_IPN_Utils::getThisNode(vector<string> &ipdot_vec)
{
    bool retCode = false;
    ACS_DSD_Server s(acs_dsd::SERVICE_MODE_INET_SOCKET_PRIVATE);
    ACS_DSD_Node my_node;
    s.get_local_node(my_node);

    string apName(my_node.node_name);

    // convert to capital letters.
    for (unsigned int i = 0; i < apName.size(); i++)
    {
        apName[i] = char(toupper(apName[i]));
    }

    for (int apnr=0;apnr<NO_OF_AP;apnr++)
    {
         for (int apside=0;apside<2;apside++)
         {
             if (APNAMES[apnr][apside].compare(apName)==0)
             {
                 ipdot_vec.push_back(APIP[apnr][apside][0]);
                 ipdot_vec.push_back(APIP[apnr][apside][1]);
                 retCode=true;
                 break;
             }
         }

         if (retCode == true)
             break;
    }

    return retCode;
}
