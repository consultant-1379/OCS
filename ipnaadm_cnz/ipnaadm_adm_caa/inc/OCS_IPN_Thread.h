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
// OCS_IPN_Thread.h
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

#ifndef OCS_IPN_Thread_h
#define OCS_IPN_Thread_h

#include "OCS_IPN_Global.h"
#include "OCS_IPN_Sig.h"
#include "Event.h"
#include "CriticalSection.h"


#define SENDBUFSIZE  65536
#define DBUFSIZE     65536                     // Size of data buffer

#define HEARTBEAT_TIMEOUT  60            // Timeout for receiving heartbeat is 1 minute
#define CON_TIMEOUT       300           // 5 minute timeout for IPNA initial connection


struct thread_data                         // Pass these to kill thread routine
{
   int ipnano;
   int thd_sock;
   Event dummy_event;
   Event ThreadSockEvents;
   Event Command_Event;
};

class OCS_IPN_Thread
{
public:
	/****************************************************************************
	 * Method:	constructor
	 * Description:
	 * Param [in]: N/A
	 * Param [out]: N/A
	 * Return: N/A
	 *****************************************************************************
	 */
	OCS_IPN_Thread();

	/****************************************************************************
	 * Method:	destructor
	 * Description:
	 * Param [in]: N/A
	 * Param [out]: N/A
	 * Return: N/A
	 *****************************************************************************
	 */
	~OCS_IPN_Thread();

	/****************************************************************************
	 * Method:	start()
	 * Description: Use to start IPN Thread
	 * Param [in]: N/A
	 * Param [out]: N/A
	 * Return: N/A
	 *****************************************************************************
	 */
	void start();

	/****************************************************************************
	 * Method:	stop()
	 * Description: Use to stop IPN Thread
	 * Param [in]: N/A
	 * Param [out]: N/A
	 * Return: N/A
	 *****************************************************************************
	 */
	void stop() const;

	/****************************************************************************
	 * Method:	setNewSocket()
	 * Description: Use to socket file descriptor for the IPN Thread
	 * Param [in]: N/A
	 * Param [out]: N/A
	 * Return: error code
	 *****************************************************************************
	 */
	void setNewSocket(int newSocket);

	/****************************************************************************
	* Method:	isStopped
	* Description: Check if thread is stopped or not
	* Param [in]: N/A
	* Param [out]: N/A
	* Return: N/A
	*****************************************************************************
	*/
	bool isStopped() const ;

	/****************************************************************************
	* Method:	fileOlder
	* Description: Compare two files to see which one is older
	* Param [in]: file1 - first file
	* Param [in]: file1 - second file
	* Param [out]: N/A
	* Return: true if file1 older than file2 and false otherwise
	*****************************************************************************
	*/
	static bool fileOlder(char *file1, char *file2);

	/****************************************************************************
	* Method:	prepareFunctionChange
	* Description: prepare boot.ipn# for function change
	* Param [in]: ipnx - ipna #
	* Param [out]: N/A
	* Return:
	*        CMDFCPREP_SUCCESS(0) : Operation succeeded
	*        CMDFCPREP_NOCHANGE(1): No Function Change files loaded
	*        CMDFCPREP_FAULT(2)   : Other fault
	*****************************************************************************
	*/
	static uint32_t prepareFunctionChange(char ipnx);

	/****************************************************************************
	* Method:	connectToCPon14000
	* Description: Connect to IPNAs at port 14000
	* Param [in]: net
	* Param [out]: N/A
	* Return: true or false
	*****************************************************************************
	*/
	static bool connectToCPon14000(uint16_t net);

	// Declare the static data members

	static int    s_commandConnected;               // Indication of command handler connection
	static int    s_commandData[MAX_IPNAS];         // Command data from command handler
	static int    s_replyData[MAX_IPNAS];           // Reply data to command handler
	static bool   s_replied[MAX_IPNAS];              // IPNA replied indication array
	static int    s_connected[MAX_IPNAS];            // IPNA connection array
	static uint32_t s_linkIP[2];


	static Event s_threadTermEvent;
	static Event s_commandEvents[MAX_IPNAS];       // Command events from command handler
	static Event s_ipnaRepliedEvent;              // Reply event to command handler
	static Event s_multiConTermEvent[MAX_IPNAS];    // Events for killing multiple IPNA connections
	static Event s_multiCmdTermEvent;               // Event for killing multiple command connections
	static Event s_stopEvent;

	static long   s_noConns;                    // Number of connections made since service started
	static int    s_currCons;                   // Number of current connections

	static CriticalSection s_connSec;           // Critical section for changing curr_cons
	static CriticalSection s_ipnaSec;
	static CriticalSection m_critSec;


private:
	/****************************************************************************
	* Method:	makeItSend
	* Description: Send message to client
	* Param [in]: its_sock - socket to send data
	* Param [in]: its_buf - data to send
	* Param [in]: its_len - data lengh to send
	* Param [in]: its_name - name of data
	* Param [in]: ipnano - ipna #
	* Param [out]: N/A
	* Return: true or false
	*****************************************************************************
	*/
	bool makeItSend(int its_sock,char *its_buf,int its_len, const char *its_name,int ipnano);

	/****************************************************************************
	* Method:	recvItAll
	* Description: receive message from client
	* Param [in]: its_sock - socket to receiv data
	* Param [in]: its_buf - buffer to receive data
	* Param [in]: its_len - maximum data to receive
	* Param [in]: its_name - name of data
	* Param [in]: ipnano - ipna #
	* Param [out]: N/A
	* Return: true or false
	*****************************************************************************
	*/
	bool recvItAll(int its_sock,char *its_buf,int its_len,const char *its_name,int ipnano);

	/****************************************************************************
	* Method:	killThread
	* Description: kill thread
	* Param [in]: err_code - error code
	* Param [in]: thread_data - thread infromation
	* Param [out]: N/A
	* Return: N/A
	*****************************************************************************
	*/
	void killThread(int err_code,struct thread_data *thread_data);


private:
	int m_newSocket;
	bool m_stop;	
};

#endif
