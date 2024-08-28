//******************************************************************************
// COPYRIGHT Ericsson Utvecklings AB, Sweden 2000,2011
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
// OCS_IPN_Cmd.h
//
// DESCRIPTION
// This file contains local prototypes and definitions.
//
// DOCUMENT NO
// 9/190 55-CAA 109 1405
//
// AUTHOR
// 20000927 EPA/D/U Michael Bentley
// 2011xxxx XDT/DEK Rex Barnett
// 20110815 XDT/DEK Danh Nguyen
//
//******************************************************************************
// *** Revision history ***
// 20000927 MJB Created
// 2011xxxx Rex Migrated from Win32 to APG43 on Linux
// 20110810 Danh Updated
//******************************************************************************

#ifndef OCS_IPN_Cmd_h
#define OCS_IPN_Cmd_h

#include "OCS_IPN_Sig.h"                    // Signal definitions
#include "OCS_IPN_Global.h"

/*****************************************************************************/

#define  ADM_PORT_NUMBER        2346          // Server port no.
#define  MAX_IPNAS                64          // Maximum number of IPNAS

#define  MAXRETRY               1000          // Maximum number of retries of blocked recv
#define  CmdTimeout               5          // 5 seconds timeout for receiving command replies


class OCS_IPN_Cmd
{
public:

    /*********************************************************************
     * Method:      constructor
     * Description:
     * Param [in] : num, *myArg[]
     * Param [out]: N/A
     * Return     : N/A
     *********************************************************************
     */
    OCS_IPN_Cmd(int num, char *myArg[]);

    /*********************************************************************
    * Method:      destructor
    * Description:
    * Param [in] : N/A
    * Param [out]: N/A
    * Return     : N/A
    *********************************************************************
    */
    ~OCS_IPN_Cmd();

    //******************************************************************************
    // Function Member Prototypes
    //******************************************************************************

    /*********************************************************************
     * Method:      run
     * Description: Running communication between server and command client
     * Param [in] : N/A
     * Param [out]: N/A
     * Return     : error code
     *********************************************************************
     */

     int run();

private:

     /*********************************************************************
      * Method:      usage()
      * Description: Print a help message
      * Param [in] : N/A
      * Param [out]: N/A
      * Return     : N/A
      *********************************************************************
      */

    void usage(void);

    /*********************************************************************
    * Method:      serverConnect
    * Description: Connect to the IPNAADM server
    * Param [in] : N/A
    * Param [out]: N/A
    * Return     : N/A
    *********************************************************************
    */

    int serverConnect(void);

    /*********************************************************************
    * Method:      makeItSend
    * Description: Send a signal and make sure it's all sent
    * Param [in] : N/A
    * Param [out]: N/A
    * Return     : error code
    *********************************************************************
    */

    bool makeItSend(int its_sock,char *its_buf,int its_len, char *its_name);

    /*********************************************************************
    * Method:      recvItAll
    * Description: Receive multiple bytes from a int and ensure it is
    *              all received
    * Param [in] : N/A
    * Param [out]: N/A
    * Return     : N/A
    *********************************************************************
    */

    bool recvItAll(int its_sock,char *its_buf,int its_len,char *its_name);

    /*********************************************************************
    * Method:      cleanUp
    * Description: Clean up and die
    * Param [in] : N/A
    * Param [out]: N/A
    * Return     : N/A
    *********************************************************************
    */

    void cleanUp(int my_sock);

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

    int32_t rollBackFunctionChange(char ipnx);

    /*********************************************************************
    * Method:      printSignal
    * Description: Print the signal pointed at by signal
    * Param [in] : N/A
    * Param [out]: N/A
    * Return     : N/A
    *********************************************************************
    */

    void printSignal(void);

    /*********************************************************************
    * Method:      syntError
    * Description: Print the syntax error
    * Param [in] : N/A
    * Param [out]: N/A
    * Return     : N/A
    *********************************************************************
    */

    void syntError(void);

    /*********************************************************************
     * Method:      sendError
     * Description: Print the communication error
     * Param [in] : N/A
     * Param [out]: N/A
     * Return     : N/A
     *********************************************************************
     */

     void sendError(void);

private:

    char     rec_buf[16];                       // Buffer big enough to receive signal head
    char     send_buf[100];                     // Small send buffer

    int      cmd_sock;                          // Command socket
    int      s_argc;
    char     **s_argv;
};

#endif
