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
// OCS_IPN_Server.h
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

#ifndef OCS_IPN_SERVER_H_
#define OCS_IPN_SERVER_H_

#include "OCS_IPN_Global.h"
#include "OCS_IPN_Sig.h"
#include "Event.h"
#include <boost/thread.hpp>

#include <map>

#define LIST_BACKLOG 10                       // Backlog of connections for listen

class OCS_IPN_Thread;
class OCS_IPN_Service;

typedef boost::shared_ptr<OCS_IPN_Thread> OCS_IPN_Thread_Ptr;
typedef boost::shared_ptr<boost::thread> boost_thread_Ptr;

class OCS_IPN_Server
{
public:
	/****************************************************************************
	 * Method:	constructor
	 * Description:
	 * Param [in]: N/A
	 * Param [out]: N/A
	 * Return: error code
	 *****************************************************************************
	 */
	OCS_IPN_Server();

	/****************************************************************************
     * Method:  constructor
     * Description:
     * Param [in]: N/A
     * Param [out]: N/A
     * Return: error code
     *****************************************************************************
     */
    OCS_IPN_Server(OCS_IPN_Service* pOCSIPNService);

	/****************************************************************************
	 * Method:	destructor
	 * Description:
	 * Param [in]: N/A
	 * Param [out]: N/A
	 * Return: error code
	 *****************************************************************************
	 */
	~OCS_IPN_Server();

	/****************************************************************************
	 * Method:	start()
	 * Description: Use to start IPN server
	 * Param [in]: N/A
	 * Param [out]: N/A
	 * Return: error code
	 *****************************************************************************
	 */
	int start();

	/****************************************************************************
	 * Method:	stop()
	 * Description: Use to stop IPN server
	 * Param [in]: N/A
	 * Param [out]: N/A
	 * Return: error code
	 *****************************************************************************
	 */
	void stop();

	/****************************************************************************
	* Method:	terminateServer()
	* Description: Use to stop IPNAADM server
	* Param [in]: why - cause to terminate
	* Param [in]: exitCode - exit code
	* Param [out]: N/A
	* Return: N/A
	*****************************************************************************
	*/
	void terminateServer(const char *why, int exitCode);

	/****************************************************************************
	* Method:	setRunningMode()
	* Description: Set running mode.
	* Param [in]: mode - 1: AMF; 2: noservice
	* Param [out]: N/A
	* Return: N/A
	*****************************************************************************
	*/
	void setRunningMode(int mode);

private:

	/****************************************************************************
     * Method:  cleanup()
     * Description: Use to clean up the server
     * Param [in]: N/A
     * Param [out]: N/A
     * Return: N/A
     *****************************************************************************
     */
	void cleanup();

	/****************************************************************************
     * Method:  bind()
     * Description: Use to bind a socket to a sock_addr
     * Param [in]: socket - socket descriptor
     * Param [in]: address - sip address
     * Param [in]: port - TCP port
     * Return: tru if success, false otherwise
     *****************************************************************************
     */
	bool bind(const int socket, const char* address, const uint16_t port);

    // Prevent the user from trying to use value semantics on this object
	OCS_IPN_Server(const OCS_IPN_Server &server);
	void operator=(const OCS_IPN_Server &server);

	//Server socket
    int m_listenSocket[3];
    
    //Server port
    uint16_t m_serverPort;

	bool m_stop;
	static Event s_stopEvent;
    std::map<int , OCS_IPN_Thread_Ptr> m_ocsIPNThreadMap;
    std::map<int , boost_thread_Ptr> m_threadMap;
    int m_runningMode; // 1: AMF service, 2: no service

    // Pointer to amf service object
    OCS_IPN_Service* m_pOCSIPNService;


};

#endif /* OCS_IPN_SERVER_H_ */
