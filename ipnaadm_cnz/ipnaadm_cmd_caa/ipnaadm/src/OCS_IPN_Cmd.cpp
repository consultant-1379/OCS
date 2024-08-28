//******************************************************************************
// COPYRIGHT Ericsson Utvecklings AB, Sweden 2000,2011.
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
// OCS_IPN_Cmd.cpp
//
// DESCRIPTION
//
// DOCUMENT NO
// 8/190 55-CAA 109 1405
//
// AUTHOR
// 20000926 EPA/D/U Michael Bentley
// 2011xxxx XDT/DEK Rex Barnett
// 20110801 XDT/DEK Danh Nguyen
//
//******************************************************************************
// *** Revision history ***
// 20000926 MJB Created
// 2011xxxx Rex Migrated from Win32 to APG43 on Linux
// 20110810 Danh Updated
//******************************************************************************


#include "OCS_IPN_Cmd.h"
#include "OCS_IPN_ExtFnx.h"
#include "OCS_IPN_Utils.h"
#include "OCS_IPN_Trace.h"
#include "ACS_CS_API.h"

#include <acs_prc_api.h>

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <ctype.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <dirent.h>
#include <linux/sockios.h>
#include <errno.h>
#include <sstream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <list>
#include <cctype>


using namespace std;


/******************************************************************************
 *
 *
 * IPNA Administration command handler
 *
 * INPUT:  -
 * OUTPUT:
 * OTHER:  -
 ******************************************************************************
 */

/*********************************************************************
 * Method:      main
 * Description:
 * Param [in] : argc, argv
 * Param [out]: N/A
 * Return     : error code
 *********************************************************************
 */

int main(int argc, char *argv[])
{
    // No TRACE shall be invoked before this point
    initTRACE();

    int ret_val;

    OCS_IPN_Cmd *OCS_IPN_command = new OCS_IPN_Cmd(argc, argv);

    ret_val = OCS_IPN_command->run();

    delete OCS_IPN_command;

    return ret_val;
}


/*********************************************************************
 * Method:      constructor
 * Description:
 * Param [in] : num, *myArg[]
 * Param [out]: N/A
 * Return     : N/A
 *********************************************************************
 */
OCS_IPN_Cmd::OCS_IPN_Cmd(int num, char *myArg[]):
        s_argc(num)
{
    // Fill zero to rec_buf & send_buf buffers
    memset (rec_buf,0,sizeof(rec_buf));
    memset (send_buf,0,sizeof(send_buf));

    // Set cmd_sock to invalid socket descriptor
    cmd_sock = -1;

    s_argv = new char*[s_argc];
    for (int i = 0; i < s_argc ; i++)
    {
        s_argv[i] = new char[100];
        strcpy(s_argv[i], myArg[i]);
    }
}


/*********************************************************************
 * Method:      destructor
 * Description:
 * Param [in] : N/A
 * Param [out]: N/A
 * Return     : N/A
 *********************************************************************
 */
OCS_IPN_Cmd::~OCS_IPN_Cmd()
{

    for (int i = 0; i < s_argc ; i++)
    {
        delete s_argv[i];
    }

    delete [] s_argv;
}


/*********************************************************************
 * Method:      run
 * Description: Running communication between server and command client
 * Param [in] : N/A
 * Param [out]: N/A
 * Return     : error code
 ********************************************************************
 */

int OCS_IPN_Cmd::run()
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_Cmd::run()",0));

    int result = 0;             // Result from function
    ssize_t nbRead;             // Number of bytes read from recv
    int rc = 0;                     // Return code
    int errorCode = 0;

    char *datap;                // Pointer to start of received data
    char ch;

    int i, fnlen;
    int texit = 0;
    uint32_t nBytes, k;
    char state;                 // Signal parameters
    int ipnano =-1;
    int nOctets;

    char lstring[65535];        // String
    uint32_t slen;

    int retrycnt;               // Counter for number of retries of recv

    timeval tv;

    //--------------------------------------------------------------------------------------------------
    // Events
    //--------------------------------------------------------------------------------------------------

    fd_set master;                      // Master file description list
    fd_set fds;                         // Temporary file description list

    bool classicAPZ = false;
    bool multiCP = false;

    ACS_CS_API_NS::CS_API_Result returnValue;

    returnValue = ACS_CS_API_NetworkElement::isMultipleCPSystem(multiCP);

    // Check if the APZ system is classic
    if (returnValue == ACS_CS_API_NS::Result_Success)
    {
        if (multiCP == false)
        {
            classicAPZ = OCS_IPN_Utils::isClassicCP();
        }
    }

    if (classicAPZ == false)
    {
    	cout << "APZ system is non-classic" << endl;
    	return -1;
    }

    // Check if the AP node is passive
    if(!OCS_IPN_Utils::isActiveNode())
    {
        cout << "Must run on active node" << endl;
        return -1;
    }


    if (s_argc == 1)
    {
        syntError();                    // There are no arguments, just print help
        return 2;
    }
    if (s_argc == 2)
    {
        // Test single argument commands
        if ((strcmp(s_argv[1], "-help") == 0) || (strcmp(s_argv[1], "-h") == 0) || (strcmp(s_argv[1], "/?") == 0))
        {
            usage();                    // Help command - print help message
            return 2;
        }
        else if (strcmp(s_argv[1], "-list") == 0)
        {
            if (serverConnect() < 0)   // Connect to the IPNAADM server
            {
                TRACE((LOG_LEVEL_INFO, "Connect to IPNAADM server failed",0));
                return -1;
            }
            // list command (no args)
            // Send CMDOSDLIST

            ((struct cmdssreq_sig *) send_buf)->sh.sig_len = htons(SL_CMDLIST);
            ((struct cmdssreq_sig *) send_buf)->sh.sig_num = CMDLISTREQ;
            ((struct cmdssreq_sig *) send_buf)->sh.sig_ver = 0;
            if (!makeItSend(cmd_sock, send_buf, SL_CMDLIST+sizeof(struct sig_head), (char*) "CMDLISTREQ"))
            {
                sendError();       // Exit on error
                return 21;
            }

        }
        else if (strcmp(s_argv[1], "-show") == 0)
        {
            //List all configuration file at boot folder location (/data/apz/data/)
            string bootFolder(TFTPBOOTDIR);
            vector<string> configurationFiles;
            vector<string>::iterator it;

            errorCode = OCS_IPN_Utils::getConfigurationFiles(configurationFiles, bootFolder);
            if( errorCode != 0)
            {
                printf("Show configuration files failed with error %d", errorCode);
                return 23;
            }

            // Sort the file list
            sort(configurationFiles.begin(), configurationFiles.end());

            printf("List of configuration files:\n\n");
            it = configurationFiles.begin();
            while (it != configurationFiles.end())
            {
                printf("%s\n",(*it).c_str());
                ++it;
            }

            printf("\nendlist\n");
            return 0;
        }
        else
        {   // Command not recognised - print usage
            syntError();
            return 2;

        }


    }
    else if (s_argc == 3)
    {
        if ((strcmp(s_argv[1], "-osdump") == 0) &&
                (strcmp(s_argv[2], "-list") == 0))
        {
            if (serverConnect() < 0)       // Connect to the IPNAADM server
            {
                TRACE((LOG_LEVEL_INFO, "Connect to IPNAADM server failed",0));
                return -1;
            }

            // Handle the list sub-command, ask server for a list of dump files

            ((struct cmdosdreq_sig *) send_buf)->sh.sig_len = htons(SL_CMDOSD_LIST);
            ((struct cmdosdreq_sig *) send_buf)->sh.sig_num = CMDOSDREQ;
            ((struct cmdosdreq_sig *) send_buf)->sh.sig_ver = 0;
            ((struct cmdosdreq_sig *) send_buf)->cmd = CMDOSD_LIST;

            if (!makeItSend(cmd_sock, send_buf, SL_CMDOSD_LIST+sizeof(struct sig_head), (char*) "CMDOSDREQ -list"))
            {
                sendError();       // Exit on error
                return 21;
            }


        }
        else if (strcmp(s_argv[1], "-del") == 0)
        {
                // Verify if s_argv[2] is *.ipn* file
                if(!OCS_IPN_Utils::isConfigurationFile(s_argv[2]))
                {
                    printf("File %s is not a configuration file\n", s_argv[2]);
                    return 22;
                }
                // Delete backup.ipnx file
                string file = string(TFTPBOOTDIR) + string(s_argv[2]);
                if(remove(file.c_str()) != 0)
                {
                    printf("Delete file failed with error: %s\n", strerror(errno));
                    return 25;
                }
                else
                {
                    printf("Delete %s, successful\n", s_argv[2]);
                    printf("\nendlist\n");
                    return 0;
                }
        }
        else
        {    // Command not recognized - print usage
            syntError();
            return 2;
        }


    }
    else if (s_argc == 4)
    {
        if ((strcmp(s_argv[1], "-osdump") == 0) &&
                (strcmp(s_argv[2], "-get")== 0))
        {
            if (serverConnect() < 0)       // Connect to the IPNAADM server
            {
                TRACE((LOG_LEVEL_INFO, "Connect to IPNAADM server failed",0));
                return -1;
            }

            // Get <filename> - send CMDOSDREQ(get,filename)

            fnlen = strlen(s_argv[3]);   // Length of filename

            ((struct cmdosdreq_sig *) send_buf)->sh.sig_len = htons(SL_CMDOSD_GET + fnlen);
            ((struct cmdosdreq_sig *) send_buf)->sh.sig_num = CMDOSDREQ;
            ((struct cmdosdreq_sig *) send_buf)->sh.sig_ver = 0;
            ((struct cmdosdreq_sig *) send_buf)->cmd = CMDOSD_GET;

            datap = &((struct cmdosdreq_sig *) send_buf)->filenam; // Pointer to filename in signal
            strcpy(datap, s_argv[3]);    // Copy filename into signal

            TRACE((LOG_LEVEL_INFO, "Getting file %s",0,s_argv[3]));
            //printSignal();

            if (!makeItSend(cmd_sock, send_buf, SL_CMDOSD_GET+sizeof(struct sig_head) + fnlen, (char*) "CMDOSDREQ -get"))
            {
                sendError();       // Exit on error
                return 21;
            }

        }

        else if ((strcmp(s_argv[1], "-fcprep") == 0) &&
                    (strcmp(s_argv[2],"-ipnano") == 0))
        {
            result = sscanf(s_argv[3], "%d", &ipnano);
            if ((result != 1) || (ipnano > 63) || (ipnano < 0))
            {
                cout << "Error in ipna number argument " << s_argv[3] << endl; // sscanf didn't match 1 number
                usage();
                cleanUp(cmd_sock);          // Exit on error
                return 3;
            }

            if (serverConnect() < 0)       // Connect to the IPNAADM server
            {
                TRACE((LOG_LEVEL_INFO, "Connect to IPNAADM server failed",0));
                return -1;
            }

            ((struct cmdfcprepreq_sig *) send_buf)->sh.sig_len = htons(SL_CMDFCPREPREQ);
            ((struct cmdfcprepreq_sig *) send_buf)->sh.sig_num = CMDFCPREPREQ;
            ((struct cmdfcprepreq_sig *) send_buf)->sh.sig_ver = 0;
            ((struct cmdfcprepreq_sig *) send_buf)->ipnano = ipnano;

            if (!makeItSend(cmd_sock, send_buf, SL_CMDFCPREPREQ+sizeof(struct sig_head), (char*) "FC Preparation"))
            {
                sendError();       // Exit on error
                return 21;
            }

        }

        else if ((strcmp(s_argv[1], "-fcrollb") == 0) &&
                    (strcmp(s_argv[2],"-ipnano") == 0))
        {
            result = sscanf(s_argv[3], "%d", &ipnano);
            if ((result != 1) || (ipnano > 63) || (ipnano < 0))
            {
                cout << "Error in ipna number argument " << s_argv[3] << endl; // sscanf didn't match 1 number
                usage();
                cleanUp(cmd_sock);   // Exit on error
                return 3;
            }

            switch (rollBackFunctionChange(ipnano))
            {
            case CMDFCPREP_SUCCESS:
               cout << "\n\nCommand succeeded\n";
               texit = 0;
               break;
            case CMDFCPREP_NOROLLBACK:
               cout << "\n\nRollback not possible\n";
               texit = 13;
               break;
            default:
               cout << "\n\nCommand returned unknown value " << ((struct cmdfcprepconf_sig *) rec_buf)->result << endl;
               sendError();       // Exit on error
               return 21;
               break;
            }
            return texit;
        }
        else if (strcmp(s_argv[1], "-copy") == 0)
        {
            // Verify if s_argv[2] is *.ipn* file
            if(!OCS_IPN_Utils::isConfigurationFile(s_argv[2]))
            {
                printf("File %s is not a configuration file\n", s_argv[2]);
                return 22;
            }
            // Verify if s_argv[3] is *.ipn* file
            if(!OCS_IPN_Utils::isConfigurationFile(s_argv[3]))
            {
                printf("File %s is not a configuration file\n", s_argv[3]);
                return 22;
            }

            string srcFile = string(TFTPBOOTDIR) + string(s_argv[2]);
            string dstFile = string(TFTPBOOTDIR) + string(s_argv[3]);
            errorCode = OCS_IPN_Utils::copyBootFile(srcFile.c_str(),dstFile.c_str());
            if( errorCode != 0)
            {
                printf("Copy file failed with error: %s\n", strerror(errorCode));
                return 24;
            }
            else
            {
               printf("Copy %s %s, successful\n",s_argv[2], s_argv[3]);
               printf("\nendlist\n");
               return 0;
            }
        }
        else if (strcmp(s_argv[1], "-ren") == 0)
        {
            // Verify if s_argv[2] is *.ipn* file
            if(!OCS_IPN_Utils::isConfigurationFile(s_argv[2]))
            {
                printf("File %s is not a configuration file\n", s_argv[2]);
                return 22;
            }
            // Verify if s_argv[3] is *.ipn* file
            if(!OCS_IPN_Utils::isConfigurationFile(s_argv[3]))
            {
                printf("File %s is not a configuration file\n", s_argv[3]);
                return 22;
            }

            string oldFile = string(TFTPBOOTDIR) + string(s_argv[2]);
            string newFile = string(TFTPBOOTDIR) + string(s_argv[3]);
            if(rename(oldFile.c_str(), newFile.c_str()) != 0)
            {
                printf("Rename file failed with error: %s\n", strerror(errno));
                return 26;
            }
            else
            {
                printf("Rename %s %s, successful\n",s_argv[2], s_argv[3]);
                printf("\nendlist\n");
                return 0;
            }
        }
        else
        {
            syntError();
            return 2;
        }
    }
    else if (s_argc == 5)
    {
        if (strcmp(s_argv[1], "-state") == 0)
        {
            // State command args:state<norm(al),sep(erated)> ipna<0..63>

            // Check argument to state - allow both cases, only look at 1st few chars

            if (strncmp(s_argv[2], "normal", 4) == 0)
            {
                state = CMDSS_NORM;
            }
            else if (strncmp(s_argv[2], "separated", 3) == 0)
            {
                state = CMDSS_SEP;
            }
            else
            {
                cout << "Error in state command argument " << s_argv[2] << endl;
                usage();
                cleanUp(cmd_sock);
                return 3;
            }

            // Check for ipna number argument next

            if (strcmp(s_argv[3], "-ipnano") == 0)
            {
                result = sscanf(s_argv[4], "%d", &ipnano);
                if ((result != 1) || (ipnano > 63) || (ipnano < 0))
                {
                    cout << "Error in ipna number argument " << s_argv[4] << endl; // sscanf didn't match 1 number
                    usage();
                    cleanUp(cmd_sock);      // Exit on error
                    return 3;
                }
            }
            else
            {
                syntError();
                return 2;
            }

            if (serverConnect() < 0)       // Connect to the IPNAADM server
            {
                TRACE((LOG_LEVEL_INFO, "Connect to IPNAADM server failed",0));
                return -1;
            }

            // Send CMDSSREQ

            ((struct cmdssreq_sig *) send_buf)->sh.sig_len = htons(SL_CMDSSREQ);
            ((struct cmdssreq_sig *) send_buf)->sh.sig_num = CMDSSREQ;
            ((struct cmdssreq_sig *) send_buf)->sh.sig_ver = 0;
            ((struct cmdssreq_sig *) send_buf)->ipnano = ipnano;
            ((struct cmdssreq_sig *) send_buf)->state = state;
            ((struct cmdssreq_sig *) send_buf)->substate = CMDSS_OPEN;

            if (!makeItSend(cmd_sock, send_buf, SL_CMDSSREQ+sizeof(struct sig_head), (char*) "CMDSSREQ"))
            {
                sendError();       // Exit on error
                return 21;
            }

        }
        else
        {
            syntError();
            return 2;
        }

    }
    else
    {
        syntError();
        return 2;
    }

    tv.tv_sec = CmdTimeout;         // Initialize timeout
    tv.tv_usec = 0;

    FD_ZERO(&master);
    FD_ZERO(&fds);
    FD_SET(cmd_sock, &master);      // Command Socket

    //----------------------------------------------------------------------------------
    // Enter loop waiting for network events, the first should be the IDENTITYREQ signal
    //----------------------------------------------------------------------------------

    while (true)
    {

        fds = master;
        rc = ::select(cmd_sock + 1, &fds, 0, 0, &tv);

        if (rc < 0)
        {
            cout << "Failed server select with error " << strerror(errno) << endl;
            cleanUp(cmd_sock); // Exit on error
            return -1;
        }
        else if (rc == 0)
        {
            // timeout waiting for network events
            cout << "\n\n  Error waiting for network events\n";
            cleanUp(cmd_sock);      // Exit on timeout
            return 20;
        }

        if (FD_ISSET(cmd_sock,&fds))
        {
            TRACE((LOG_LEVEL_INFO, "Handling socket event",0));
            ioctl(cmd_sock, FIONREAD, &nOctets);    // Is there is something to read?
            if (nOctets > 0)
            {

                if (!recvItAll(cmd_sock, rec_buf, sizeof(sig_head),(char*) "Signal Header"))
                {
                    cout << "Read of signal header failed with error " << strerror(errno) << endl;
                    cleanUp(cmd_sock);
                    return 18;
                }

                switch (((struct sig_head *) rec_buf)->sig_num)
                {
                case CMDSSCONF:
                {
                    TRACE((LOG_LEVEL_INFO, "Received CMDSSCONF",0));
                    // Print result of set state

                    nbRead = recv(cmd_sock, &ch, 1, 0);     // Receive dump sub-command from signal
                    if (nbRead == -1 || nbRead != 1)
                    {
                        cout << "Receive of dump sub-command failed with error " << strerror(errno) << endl;
                        cleanUp(cmd_sock);
                        return 17;
                    }
                    switch (ch)
                    {
                    case CMDSS_SUCCESS:
                        cout <<"\n\nCommand succeeded\n";
                        texit = 0;
                        break;

                    case CMDSS_UNKNOWN:
                        cout <<"\n\nIPNA reports unknown state\n";
                        texit = 4;
                        break;

                    case CMDSS_EX_CP:
                        cout <<"\n\nIPNA reports order received in EX-CP state\n";
                        texit = 5;
                        break;

                    case CMDSS_NOTCON:
                        cout << "\n\nIPNA is not connected\n";
                        texit = 6;
                        break;

                    default:
                        cout << "\n\nCommand returned unknown value " << ((struct cmdssconf_sig *) rec_buf)->result << endl;
                        texit = 16;
                        break;
                    }
                    cleanUp(cmd_sock);       // End of command
                    return texit;
                    break;
                }

                case CMDFCPREPCONF:
                {
                    TRACE((LOG_LEVEL_INFO, "Received CMDFCPREPCONF",0));
                    // Print result of function change preparation

                    nbRead = recv(cmd_sock, &ch, 1, 0); // Receive nbRead from signal
                    if (nbRead == -1 || nbRead != 1)
                    {
                        printf("Receive of FC preparation conf, failed with error %d\n",errno);
                        cleanUp(cmd_sock);
                        return 14;
                    }

                    TRACE((LOG_LEVEL_INFO, "Received CMDFCPREPCONF with result = %d",0,ch));

                    switch (ch)
                    {
                    case CMDFCPREP_SUCCESS:
                        printf("\n\nCommand succeeded\n");
                        texit = 0;
                        break;

                    case CMDFCPREP_NOCHANGE:
                        printf("\n\nNo Function Change files loaded\n");
                        texit = 12;
                        break;

                    default:
                        printf("\n\nCommand returned unknown value %d\n",((struct cmdfcprepconf_sig *) rec_buf)->result);
                        texit = 16;
                        break;
                    }
                    cleanUp(cmd_sock); // End of command
                    return texit;
                    break;
                }
                case CMDLISTCONF:
                {
                   TRACE((LOG_LEVEL_INFO, "Received CMDLISTCONF",0));

                    printf("\n\nConnected IPNAs are :\n\n");
                    for (i = 0; i < MAX_IPNAS; i++)
                    {
                        nbRead = recv(cmd_sock, &ch, 1, 0);     // Read one connection list entry
                        if (nbRead == -1 || nbRead != 1)
                        {
                            printf("Receive of dump sub-command failed with error %d\n",errno);
                            cleanUp(cmd_sock);
                            return 17;
                        }
                        if (ch > 0)
                        {
                            for (int m = 0; m < ch; m++)
                            { // Might have multiple connections !
                                printf("    ipna%02d ", i);
                                if ((i > 0) && (i % 8 == 0))
                                    printf("\n"); // Just 8 per line
                            }
                        }
                    }
                    printf("\n\nendlist\n");        // Finish line
                    return 0;                       // End of command
                    break;
                }

                case CMDOSDCONF:
                {
                    TRACE((LOG_LEVEL_INFO, "Received CMDOSDCONF",0));

                    nbRead = recv(cmd_sock, &ch, 1, 0); // Receive sub-command from socket
                    if (nbRead == -1 || nbRead != 1)
                    {
                        printf("Receive of dump sub-command failed with error %d\n",errno);
                        cleanUp(cmd_sock);
                        return 17;
                    }
                    switch (ch)
                    { // Test which sub-command made reply
                        case CMDOSD_LIST:
                        {
                            TRACE((LOG_LEVEL_INFO, "Received CMDOSD_LIST",0));

                            nbRead = recv(cmd_sock, &ch, 1, 0);     // Read command result
                            if (nbRead == -1 || nbRead != 1)
                            {
                                printf("Receive of CMDOSD_LIST result failed with error %d\n",errno);
                                cleanUp(cmd_sock);
                                return 8;
                            }
                            if (ch == CMDOSD_SUCCESS)
                            {
                                // If result !=0, there are dump files available and data contains a single
                                // string which is a list of file names separated by <CR><LF>

                                memset(lstring, 0, sizeof(lstring)); // Clear the string (terminates its too)

                                slen = ntohs(((struct sig_head *) rec_buf)->sig_len) - SL_CMDOSD_LISTR;
                                printf("\n\nList of available dump files:\n\n");

                                if (!recvItAll(cmd_sock, lstring, slen,(char*) "CMDOSD_LIST"))
                                {
                                    sendError();       // Exit on error
                                    return 21;
                                }

                                printf("%s\n", lstring);    // Print the received string

                                printf("endlist\n");        // Finish line


                            }
                            else
                            {
                                printf("\n\nThere are no dump files available\n");
                            }
                            break;
                        }
                        case CMDOSD_GET:
                        {
                            TRACE((LOG_LEVEL_INFO, "Received CMDOSD_GET",0));

                            nbRead = recv(cmd_sock, &ch, 1, 0); // Read command result
                            if (nbRead == -1 || nbRead != 1)
                            {
                                printf("Receive of CMDOSD_GET result failed with error %d\n",errno);
                                cleanUp(cmd_sock);
                                return 8;
                            }
                            if (ch == CMDOSD_SUCCESS)
                            {
                                // If result !=0, there was a dump file of the name requested by
                                // the get sub-command. The dump files are about 8K long so I'll
                                // read the whole signal and print out one char at a time

                                // sig_len is a 16 bit value so watch byte order

                                nBytes = ntohs(((struct cmdosdconf_sig *) rec_buf)->sh.sig_len) - SL_CMDOSD_GETR;

                                retrycnt = 0;

                                for (k = 0; k < nBytes; k++)
                                {
                                    nbRead = recv(cmd_sock, &ch, 1, MSG_DONTWAIT);
                                    if ((nbRead == -1) || (nbRead != 1))
                                    {
                                        TRACE((LOG_LEVEL_INFO, "This is an error. Number of bytes read is: %d",0,nbRead));

                                        //break;
                                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
                                        {
                                            if (retrycnt++ > MAXRETRY)
                                            {
                                                TRACE((LOG_LEVEL_INFO, "before break retrycnt: %d",0,retrycnt));
                                                break;      // Break out of for loop, too many retries
                                            }
                                            else
                                                k--;        // Data not ready, re-do
                                        }
                                        else
                                        {
                                            printf("recv died during osdump get with error %d\n",errno);
                                            break;
                                        }
                                    }
                                    else
                                    {
                                        putchar(ch);        // Print the char if received ok
                                        ch = '.';
                                    }
                                }
                                printf("\n\nendlist\n");    // Finish line

                                TRACE((LOG_LEVEL_INFO, "before break retrycnt: %d",0,retrycnt));
                            }
                            else
                            {
                                printf("The primary dump:\n\nFile not found\n");
                            }
                            break;
                        }
                        default:
                        {
                            TRACE((LOG_LEVEL_INFO, "Received unhandled sub-command %d",0,(int)ch));
                        }

                    }
                    cleanUp(cmd_sock);
                    return 0;

                    break;
                }
                default:
                {
                    TRACE((LOG_LEVEL_INFO, "Received unhandled signal %d",0,((struct sig_head *)rec_buf)->sig_num));
                    break;
                }
                }

            }
            else
            {
                printf("There is no data to read on the socket\n");
                cleanUp(cmd_sock);
                return 0;
            }

        }
        else
        {
            // No socket event
            printf("Connection to IPNAADM server lost, exiting...\n");
            cleanUp(cmd_sock);      // Exit on error
            return 15;

        }
    }
    return (0);

}

/*********************************************************************
 * Method:      usage()
 * Description: Print a help message
 * Param [in] : N/A
 * Param [out]: N/A
 * Return     : N/A
 *********************************************************************
 */
void OCS_IPN_Cmd::usage(void)
{
    printf("Usage:\n");
    printf("ipnaadm -copy srcfile dstfile\n");
    printf("ipnaadm -del filename\n");
    printf("ipnaadm -fcprep -ipnano ipnano\n");
    printf("ipnaadm -fcrollb -ipnano ipnano\n");
    printf("ipnaadm -list\n");
    printf("ipnaadm -osdump -list\n");
    printf("ipnaadm -osdump -get file\n");
    printf("ipnaadm -ren oldfile newfile\n");
    printf("ipnaadm -show\n");
    printf("ipnaadm -state state -ipnano ipnano\n\n");

}


/*********************************************************************
* Method:      serverConnect
* Description: Connect to the IPNAADM server
* Param [in] : N/A
* Param [out]: N/A
* Return     : N/A
*********************************************************************
*/
int OCS_IPN_Cmd::serverConnect(void)
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_Cmd::serverConnect",0));

    sockaddr_in s_cmd;          // Command socket address
    memset(&s_cmd, 0, sizeof(s_cmd));
    short cmd_port = -1;             // Port for command socket
    struct servent *servp = 0;      // Pointer to service entry
    bool wait_ident;
    int nOctets;
    int nbRead;
    int rc;                     // Return code

    //--------------------------------------------------------------------------------------------------
    // Events
    //--------------------------------------------------------------------------------------------------

    fd_set master;              // Master file description list
    fd_set fds;                 // Temp file description list
    FD_ZERO(&master);
    FD_ZERO(&fds);

    // Create command socket
    if ((cmd_sock = ::socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        cout << "Socket creation failed with error " << strerror(errno) << endl;
        return -1;
    }

    // Get port number for this service from operating system
    if ((servp = getservbyname("ipnaadmd", "tcp")) == NULL)
    {
        // Entry for ipnaadm isn't in the service file
        cmd_port = ADM_PORT_NUMBER;              // Use this as default
    }
    else
    {
        // Entry for ipnaadm service exits
        cmd_port = ntohs(servp->s_port);                       // Use OS port number
    }

    // Connect to server loopback interface

    // Specify a local endpoint address for command socket
    s_cmd.sin_family = PF_INET;
    s_cmd.sin_addr.s_addr = inet_addr("127.0.0.1");
    s_cmd.sin_port = htons(cmd_port);

    // connect to server
    rc = connect(cmd_sock, (struct sockaddr *) &s_cmd, sizeof(s_cmd));
    if (rc == -1)
    {
        if (errno == ECONNREFUSED)
        {
            cout << "Connection refused\n";
        }
        else
        {
            cout << "Connect failed with error " << strerror(errno) << endl;
        }

        cleanUp(cmd_sock);                      // Exit on error
        return -1;
    }

    // create socket events
    // Enter loop waiting for network events until received the IDENTITYREQ signal
    FD_ZERO(&master);
    FD_ZERO(&fds);
    FD_SET(cmd_sock, &master);                  // Set command Socket
    wait_ident = true;

    timeval tv;
    tv.tv_sec = 2;                             // Initialize timeout to 2 seconds
    tv.tv_usec = 0;

    while (wait_ident)
    {
        TRACE((LOG_LEVEL_INFO, "before select cmd_sock = %d",0, cmd_sock));

        fds = master;

        rc = TEMP_FAILURE_RETRY(::select(cmd_sock+1,&fds,NULL,NULL,&tv));

         TRACE((LOG_LEVEL_INFO, "after select cmd_sock = %d, rc = %d",0,cmd_sock,rc));

        if (rc < 0)
        {
            cout << "Failed server select with error " << strerror(errno) << endl;
            cleanUp(cmd_sock);      // Exit on error
            return -1;
        }
        else if (rc == 0)
        {
            // command timeout
            TRACE((LOG_LEVEL_INFO, "Command timeout",0));

            cleanUp(cmd_sock);
            return -1;
        }

        if (FD_ISSET(cmd_sock,&fds))
        {
            ioctl(cmd_sock, FIONREAD, &nOctets);
            if (nOctets > 0)
            {

                nbRead = recv(cmd_sock, rec_buf, sizeof(struct sig_head), 0); // Read whole signal
                if (nbRead == -1)
                {
                    cout << "Read of signal header failed with error " << strerror(errno) << endl;
                    cleanUp(cmd_sock);      // Exit on error
                    return -1;

                }
                if (nbRead < (int) sizeof(struct sig_head))
                {
                    cout << "Didn't read full signal header\n";
                    cleanUp(cmd_sock);      // Exit on error
                    return -1;

                }

                // Received the IDENTITYREQ signal
                if (((struct sig_head *) rec_buf)->sig_num == IDENTITYREQ)
                {

                    // Send IDENTITYCONF

                    ((struct idconf_sig *) send_buf)->sh.sig_len = htons(SL_IDENTITYCONF);
                    ((struct idconf_sig *) send_buf)->sh.sig_num = IDENTITYCONF;
                    ((struct idconf_sig *) send_buf)->sh.sig_ver = 0;
                    ((struct idconf_sig *) send_buf)->id = ID_COMMAND;

                    if (!makeItSend(cmd_sock, send_buf, SL_IDENTITYCONF+ sizeof(struct sig_head), (char*) "IDENTITYCONF"))
                    {
                        cleanUp(cmd_sock);      // Exit on error
                        return -1;
                    }

                    else
                        wait_ident = false;         // Break out of the while loop
                }
            }
        }
    }

    return 0;
}

/*********************************************************************
* Method:      makeItSend
* Description: Send a signal and make sure it's all sent
* Param [in] : N/A
* Param [out]: N/A
* Return     : N/A
*********************************************************************
*/
bool OCS_IPN_Cmd::makeItSend(int its_sock, char *its_buf, int its_len, char *its_name)
{

    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_Cmd::makeItSend",0));

    int nbSent = 0;
    int nbRemain;
    char *datap;

    nbRemain = its_len;
    datap = its_buf;

    while (nbRemain > 0)
    {

        if ((nbSent = send(its_sock, datap, nbRemain, 0)) == -1)
        {
            TRACE((LOG_LEVEL_INFO, "Send of %s failed with error %s",0,its_name,strerror(errno)));

            return false;
        }
        if (nbSent == 0)
        {
            TRACE((LOG_LEVEL_INFO, "Send of %s failed with error %s, no bytes sent",0,its_name,strerror(errno)));
            return false;
        }
        nbRemain -= nbSent;
        datap += nbSent;
    }

   TRACE((LOG_LEVEL_INFO, "Send %d bytes of the %d bytes %s signal",0,nbSent,its_len,its_name));
   return true;
}


/*********************************************************************
* Method:      recvItAll
* Description: Receive multiple bytes from a socket and ensure it is
*              all received
* Param [in] : N/A
* Param [out]: N/A
* Return     : N/A
*********************************************************************
*/
bool OCS_IPN_Cmd::recvItAll(int its_sock, char *its_buf, int its_len, char *its_name)
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_Cmd::recvItAll",0));

    int result;

    // Set timeout to 100 miliseconds
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100*1000;

    ::setsockopt(its_sock,SOL_SOCKET,SO_RCVTIMEO, &tv,sizeof(tv));
    result = recv(its_sock,its_buf,its_len, MSG_WAITALL); // Try to read all bytes asked for


    if(result == -1)
    {
        TRACE((LOG_LEVEL_INFO, "recv %s failed with error: %s",0,its_name,strerror(errno)));
        return(false);
    }
    else if(result < its_len)
    {
        TRACE((LOG_LEVEL_INFO, "recv %d bytes of %d of %s signal. %s" ,0,result,its_len,its_name,strerror(errno)));
        return(false);
    }
    else if(result == 0)
    {
        TRACE((LOG_LEVEL_INFO, "recv zero byte: %s",0,strerror(errno)));
        return(false);
    }

    return(true);
}


/*********************************************************************
* Method:      cleanUp
* Description: Clean up and die
* Param [in] : N/A
* Param [out]: N/A
* Return     : N/A
*********************************************************************
*/
void OCS_IPN_Cmd::cleanUp(int my_sock)
{
    ::shutdown(my_sock, SHUT_RDWR);
    ::close(my_sock);             // Close control socket
}


/*********************************************************************
* Method:      rollBackFunctionChange()
* Description:
* Param [in] : N/A
* Param [out]: N/A
* Return     : CMDFCPREP_SUCCESS(0)   - Operation succeeded
*              CMDFCPREP_NOCHANGE(1)  - No Function Change files loaded
*              CMDFCPREP_FAULT(2)     - Other fault
*              CMDFCPREP_NOROLLBACK(3)- No rollback possible to perform
*********************************************************************
*/
int32_t OCS_IPN_Cmd::rollBackFunctionChange(char ipnx)
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_Cmd::rollBackFunctionChange",0));

    char ffile[80];
    char ffile_backup[80];
    char ffile_oldbackup[80];
    char ffile_temp[80];

    sprintf(ffile, "%s%d", BOOTFILE, ipnx);
    sprintf(ffile_backup, "%s%d", BACKUPFILE, ipnx);
    sprintf(ffile_oldbackup, "%s%d", OLDBACKUPFILE, ipnx);
    sprintf(ffile_temp, "%s%d", TEMPBOOTFILE, ipnx);

    TRACE((LOG_LEVEL_INFO, "ffile = %s,ffile_backup = %s,ffile_oldbackup = %s,ffile_temp = %s",0,ffile,ffile_backup,ffile_oldbackup,ffile_temp));

    // if ffile_backup does not exist and ffile_oldbackup does exist
    // move ffile_oldbackup to ffile_backup

    if (!OCS_IPN_ExtFnx::fileExists(ffile_backup))
    {
        if (OCS_IPN_ExtFnx::fileExists(ffile_oldbackup))
        {
            TRACE((LOG_LEVEL_INFO, "Move ffile_oldbackup to ffile_backup ",0));

            if (rename(ffile_oldbackup, ffile_backup) != 0)
                return CMDFCPREP_NOROLLBACK;
        }
    }

    if (OCS_IPN_ExtFnx::fileExists(ffile))
    {
        // move ffile to ffile_temp
        if (OCS_IPN_ExtFnx::fileExists(ffile_temp))
        {
            TRACE((LOG_LEVEL_INFO, "Move ffile to ffile_temp ",0));

            if (remove(ffile_temp) != 0)
                return CMDFCPREP_NOROLLBACK;
        }
        if (rename(ffile, ffile_temp) != 0)
            return CMDFCPREP_NOROLLBACK;
    }

    // move ffile_backup to ffile

    if (rename(ffile_backup, ffile) != 0)
    { 
        rename(ffile_temp, ffile);
        return CMDFCPREP_NOROLLBACK;
    }
    
    TRACE((LOG_LEVEL_INFO, "Move ffile_backup to ffile ",0));
    // move ffile_oldbackup to ffile_backup

    if (OCS_IPN_ExtFnx::fileExists(ffile_oldbackup))
    {
        TRACE((LOG_LEVEL_INFO, "Move ffile_oldbackup to ffile_backup ",0));

        if (rename(ffile_oldbackup, ffile_backup) != 0)
        {
            return CMDFCPREP_NOROLLBACK;
        }
    }

    TRACE((LOG_LEVEL_INFO, "Remove ffile_temp ",0));

    remove(ffile_temp);

    return CMDFCPREP_SUCCESS;
}

/*********************************************************************
* Method:      printSignal
* Description: Print the signal pointed at by signal
* Param [in] : N/A
* Param [out]: N/A
* Return     : N/A
*********************************************************************
*/
void OCS_IPN_Cmd::printSignal(void)
{
    int sig_len;
    int i;

    sig_len = ntohs(((struct cmdosdreq_sig *) send_buf)->sh.sig_len);

    printf("Signal header : length %d, number %d, version %d\n", sig_len,
            ((struct cmdosdreq_sig *) send_buf)->sh.sig_num,
            ((struct cmdosdreq_sig *) send_buf)->sh.sig_ver);
    printf("Data :\n");
    for (i = 0; i < sig_len; i++)
    {
        printf("%0X ", send_buf[i]);
        if ((i > 0) && (i % 16 == 0))
            printf("\n");       // new line after 16 chars

    }
    printf("\n\n");

}


/*********************************************************************
* Method:      syntError
* Description: Print the syntex error
* Param [in] : N/A
* Param [out]: N/A
* Return     : N/A
*********************************************************************
*/

void OCS_IPN_Cmd::syntError(void)
{
    cout << "Syntax error in command :";
    for (int i = 0; i < s_argc; i++)
        cout << " " << s_argv[i];
    cout << endl;
    usage();
    cleanUp(cmd_sock);
}

/*********************************************************************
 * Method:      sendError
 * Description: Print the communication error
 * Param [in] : N/A
 * Param [out]: N/A
 * Return     : N/A
 *********************************************************************
 */

void OCS_IPN_Cmd::sendError(void)
{
    cout << "Server communication failure\n";
    usage();
    cleanUp(cmd_sock);
}

