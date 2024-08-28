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
// OCS_IPN_Server.cpp
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

#include "OCS_IPN_Server.h"
#include "OCS_IPN_Service.h"
#include "OCS_IPN_Thread.h"
#include "OCS_IPN_Utils.h"
#include "OCS_IPN_Trace.h"
#include "ACS_JTP.h"

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include <sys/types.h>       // For data types
#include <sys/socket.h>      // For socket(), connect(), send(), and recv()
#include <netdb.h>           // For gethostbyname()
#include <arpa/inet.h>       // For inet_addr()
#include <unistd.h>          // For close()
#include <netinet/in.h>      // For sockaddr_in
#include <iostream>
#include <time.h>

using namespace std;

// Stop event for thread
Event OCS_IPN_Server::s_stopEvent;


/****************************************************************************
 * Method:	constructor
 * Description:
 * Param [in]: N/A
 * Param [out]: N/A
 * Return: N/A
 *****************************************************************************
 */
OCS_IPN_Server::OCS_IPN_Server()
:m_serverPort(0),
 m_stop(false),
 m_runningMode(1),
 m_pOCSIPNService(0)
{
    for (int i = 0; i < 3;  ++i)
    {
        m_listenSocket[i] = -1;
    }

    OCS_IPN_Server::s_stopEvent.resetEvent();
    m_ocsIPNThreadMap.clear();
    m_threadMap.clear();
}

/****************************************************************************
 * Method:  constructor
 * Description:
 * Param [in]: pOCSIPNService - AMF service pointer
 * Param [out]: N/A
 * Return: N/A
 *****************************************************************************
 */
OCS_IPN_Server::OCS_IPN_Server(OCS_IPN_Service* pOCSIPNService)
:m_serverPort(0),
 m_stop(false),
 m_runningMode(1),
 m_pOCSIPNService(pOCSIPNService)
{
    for (int i = 0; i < 3;  ++i)
    {
        m_listenSocket[i] = -1;
    }

    OCS_IPN_Server::s_stopEvent.resetEvent();
    m_ocsIPNThreadMap.clear();
    m_threadMap.clear();
}

/****************************************************************************
 * Method:	destructor
 * Description:
 * Param [in]: N/A
 * Param [out]: N/A
 * Return: N/A
 *****************************************************************************
 */
OCS_IPN_Server::~OCS_IPN_Server()
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_Server::~OCS_IPN_Server()",0));

    cleanup();
}

/****************************************************************************
 * Method:	start()
 * Description: Use to start INP server
 * Param [in]: N/A
 * Param [out]: N/A
 * Return: error code
 *****************************************************************************
 */
int OCS_IPN_Server::start()
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_Server::start()",0));

    bool classicAPZ;
	struct hostent*	p;
	struct sockaddr_in socketAddr;	// Variables for checking IPNA calls
	struct sockaddr_in     s_control;
	socklen_t              s_ctrllen;	// Length of Ctl sock addr returned by accept
	struct servent   *servp;	// Pointer to service entry
	JTP_HANDLE* handles = 0;
	ACS_JTP_Service* S = 0;

	// Check up the IP addresses for CP devices
	OCS_IPN_Thread::s_linkIP[0] = 0;
	OCS_IPN_Thread::s_linkIP[1] = 0;
	classicAPZ = false;
	int ret = 0;
    
	try
	{

        classicAPZ = OCS_IPN_Utils::isClassicCP();

        if (classicAPZ == true)
        {
            p = gethostbyname(CP_EX_low);
            if (p != (struct hostent*) NULL)
            {
                memcpy((char *) &socketAddr.sin_addr.s_addr, p->h_addr_list[0], p->h_length);
                OCS_IPN_Thread::s_linkIP[0] = socketAddr.sin_addr.s_addr;
            }
            p = gethostbyname(CP_EX_high);
            if (p != (struct hostent*) NULL)
            {
                memcpy((char *) &socketAddr.sin_addr.s_addr, p->h_addr_list[0], p->h_length);
                OCS_IPN_Thread::s_linkIP[1] = socketAddr.sin_addr.s_addr;
            }
        }


        // Get port number for this service from operating system (/etc/services)
        if ((servp=getservbyname("ipnaadmd","tcp")) == NULL)
        {
            m_serverPort = ADM_PORT_NUMBER;
        }
        else
        {
            m_serverPort = ntohs(servp->s_port);
        }

        TRACE((LOG_LEVEL_INFO, "ocs_ipnaadmd socket port: %d",0,m_serverPort));

        int maxfd1;
        fd_set readset1, allset1;

        FD_ZERO(&allset1);

        maxfd1 = OCS_IPN_Server::s_stopEvent.getFd();

        // Add stop event to allset
        FD_SET(OCS_IPN_Server::s_stopEvent.getFd(), &allset1);

        timeval timeOutValue;
        int ret_val;

        //Register the application.
        S = new ACS_JTP_Service(const_cast<char*>(IPNAADMJTPNAME));

        while ((S != 0 ) && (S->jidrepreq() == false))
        {
            timeOutValue.tv_sec = 5;
            timeOutValue.tv_usec = 0;

            readset1 = allset1;          /* structure assignment */
            ret_val = ::select(maxfd1 +1, &readset1, NULL, NULL, &timeOutValue);

            if(ret_val == -1)// Error
            {
                TRACE((LOG_LEVEL_ERROR, "select failed. error message: %s",0,strerror(errno)));
            }
            else if(ret_val == 0)// time out
            {
                TRACE((LOG_LEVEL_ERROR, "Register JTP failed. Try another time within 5 seconds.",0));
            }
            else
            {
                //Check for stop event
                if(FD_ISSET(OCS_IPN_Server::s_stopEvent.getFd(),&readset1))
                {
                    TRACE((LOG_LEVEL_INFO, "Receive stop event when waiting for JTP registration: %s",0,strerror(errno)));
                    //terminateServer("Service Aborted",11);
                    throw 11;
                }
            }

            // Re-create the JTP service if it is failed.
            if(S != 0)
            {
                delete S;
                S = new ACS_JTP_Service(const_cast<char*>(IPNAADMJTPNAME));
            }
        }

        // Create tcp sockets used for loopback and two internal interfaces
        for (int i = 0; i < 3; ++i)
        {
            m_listenSocket[i] = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if(m_listenSocket[i] == -1)
            {
                terminateServer("BSD socket",errno);
                throw 4;
            }
        }

        // Set socket option
        int reuse=1;
        for(int i = 0; i < 3; ++i)
        {
            if(::setsockopt(m_listenSocket[i], SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1)
            {
                terminateServer("BSD socket",errno);
                throw 5;
            }
        }

        // Bind to loopback interface for ipnaadm command to connect
        if(!OCS_IPN_Server::bind(m_listenSocket[0], "127.0.0.1", m_serverPort))
        {
            throw 6;
        }

        // Get internal interfaces for this node
        vector<string> ipAddreses;
        if(!OCS_IPN_Utils::getThisNode(ipAddreses))
        {
            throw 6;
        }

        // Bind to internal interfaces
        for (int i = 0; i < 2; ++i)
        {
           if(!OCS_IPN_Server::bind(m_listenSocket[i + 1], ipAddreses[i].c_str(), m_serverPort))
           {
               throw 6;
           }
        }

        // Listen on socket
        for (int i = 0; i < 3; ++i)
        {
            if (::listen(m_listenSocket[i], LIST_BACKLOG) == -1)
            {
                terminateServer("BSD socket", errno);
                throw 7;
            }
        }

        int maxfd = 0;
        fd_set readset, allset;

        FD_ZERO(&allset);

        for (int i =0; i < 3;  ++i)
        {
            //Add listener socket to select set
            FD_SET(m_listenSocket[i], &allset);

            if(m_listenSocket[i] > maxfd)
                maxfd = m_listenSocket[i];
        }

        // Add stop event to allset
        FD_SET(OCS_IPN_Server::s_stopEvent.getFd(), &allset);

        if(OCS_IPN_Server::s_stopEvent.getFd() > maxfd)
            maxfd = OCS_IPN_Server::s_stopEvent.getFd();


        // Get listener handles from JTP
        int noOfHandles = 8;
        handles = new  JTP_HANDLE[8];
        ACS_JTP_Job     J;

        S->getHandles(noOfHandles, handles);
        for(int i =0; i < noOfHandles; ++i)
        {
            //Add handles to select set
            FD_SET(handles[i], &allset);
            if(maxfd < handles[i])
                maxfd = handles[i];
        }


        int index = 0;

        while (!m_stop )
        {
            readset = allset;          /* structure assignment */
            ret_val = ::select(maxfd+1, &readset, NULL, NULL, NULL);

            if(ret_val == -1)// Error
            {
                TRACE((LOG_LEVEL_ERROR, "select failed. error message: %s",0,strerror(errno)));
                if(errno == EINTR)// A signal was caught
                {
                    continue;
                }
                break;
            }
            else
            {
                //Check for stop event
                if(FD_ISSET(OCS_IPN_Server::s_stopEvent.getFd(),&readset))
                {
                    TRACE((LOG_LEVEL_INFO, "receive stop event: %s",0, strerror(errno)));
                    terminateServer("Service Aborted",0);
                    m_stop = true;
                }

                // Check if listen socket is set
                for (int i = 0; i < 3; ++i)
                {
                    if (FD_ISSET(m_listenSocket[i], &readset))
                    {
                        // Accept connection
                        int newConnSD = 0;

                        s_ctrllen = sizeof(s_control);

                        if ((newConnSD = ::accept(m_listenSocket[i], (sockaddr*)&s_control, &s_ctrllen)) == -1)
                        {
                            TRACE((LOG_LEVEL_ERROR, "accept failed: %s ",0,strerror(errno)));
                        }
                        else
                        {
                            //start new OCS_IPN_Thread
                            // Find exited threads, remove them from the maps
                            TRACE((LOG_LEVEL_INFO, "Start new IPN Thread with socket: %d ", 0,newConnSD));

                            map<int,OCS_IPN_Thread_Ptr>::iterator itIPNThread;
                            map<int,boost_thread_Ptr>::iterator itBoostThread;

                            for (itBoostThread = m_threadMap.begin(); itBoostThread != m_threadMap.end();)
                            {
                                itIPNThread = m_ocsIPNThreadMap.find((*itBoostThread).first);
                                if((*itIPNThread).second->isStopped())
                                {
                                    TRACE((LOG_LEVEL_INFO, "Remove index from maps: %d",0,(*itBoostThread).first));

                                    m_threadMap.erase(itBoostThread++);
                                    m_ocsIPNThreadMap.erase(itIPNThread);
                                }
                                else
                                    ++itBoostThread;
                            }

                            // Find available key. Add elements to maps with that key.
                            bool found = false;
                            index =0;

                            while (!found && index < 100)
                            {
                                if(m_ocsIPNThreadMap.count(index))
                                    index = index + 1;
                                else found = true;
                            }

                            if(found)
                            {
                                OCS_IPN_Thread_Ptr ocs_ipn_thread = OCS_IPN_Thread_Ptr(new OCS_IPN_Thread());
                                ocs_ipn_thread->setNewSocket(newConnSD);
                                boost_thread_Ptr boost_thread = boost_thread_Ptr(new boost::thread(boost::bind(&OCS_IPN_Thread::start, ocs_ipn_thread)));

                                m_ocsIPNThreadMap[index] = ocs_ipn_thread;
                                m_threadMap[index] = boost_thread;
                            }
                            else
                            {
                                    TRACE((LOG_LEVEL_INFO, "Out of thread resources. Close socket: %d",0,newConnSD));
                                    close(newConnSD);
                            }
                       }
                    }
                }

                // Check if JTP is signaled.
                for  (int i = 0; i < noOfHandles; ++i)
                {
                    if(FD_ISSET(handles[i],&readset))
                    {
                        S->accept(&J, 0);
                        if (J.State() == ACS_JTP_Job::StateConnected)
                        {
                            uint16_t action, ipna, len, res = 0;
                            char * msg;
                            if (J.jinitind(action, ipna, len, msg) == true)
                            {
                                if (J.jinitrsp(action, ipna, 0) == true)
                                {
                                    if (action == PREPARE_FC)
                                    {
                                        res = (uint16_t)OCS_IPN_Thread::prepareFunctionChange(ipna&63);
                                    }
                                    J.jresultreq(action, res, 0, 0, msg);
                                }
                            }
                        }
                    }
                }
            }
        }
	}
	catch (int exCode)
	{
	    if((exCode != 11) && (m_pOCSIPNService != 0)) // Stop when waiting for JTP registration
	        m_pOCSIPNService->componentReportError(ACS_APGCC_COMPONENT_RESTART);

	    ret = exCode;
	}
	catch (...)
	{
	    if(m_pOCSIPNService != 0)
	       m_pOCSIPNService->componentReportError(ACS_APGCC_COMPONENT_RESTART);
	}

	//Free jtp handles memory
	if(handles != 0)
	{
		delete[] handles;
	    handles = 0;
	}

	// Free JTP service
	if (S != 0)
	{
	    delete S;
	    S = 0;
	}

	return ret;
}

/****************************************************************************
 * Method:	stop()
 * Description: Use to stop main thread (IPNA server)
 * Param [in]: N/A
 * Param [out]: N/A
 * Return: N/A
 *****************************************************************************
 */
void OCS_IPN_Server::stop()
{
	OCS_IPN_Server::s_stopEvent.setEvent();
}

/****************************************************************************
 * Method:	terminateServer()
 * Description: Use to stop IPNAADM server
 * Param [in]: why - cause to terminate
 * Param [in]: exitCode - exit code
 * Param [out]: N/A
 * Return: N/A
 *****************************************************************************
 */
void OCS_IPN_Server::terminateServer(const char* why, int exitCode)
{
	newTRACE((LOG_LEVEL_INFO, "OCS_IPN_Server::terminateServer",0));
	
	TRACE((LOG_LEVEL_INFO, "terminateServer(): reason: %s, exit code: %d ",0, why, exitCode));

	OCS_IPN_Thread::s_stopEvent.setEvent();          // Kill running threads

	//::sleep(2);
	// clean up server
	cleanup();

   	// Kill this process when running in noservice mode.
	if(this->m_runningMode == 2)
	{
		kill(getpid(),SIGTERM);
	}

}

/****************************************************************************
 * Method:  setRunningMode()
  *****************************************************************************
 */
void OCS_IPN_Server::setRunningMode(int mode)
{
    this->m_runningMode = mode;
}

/****************************************************************************
 * Method:  bind()
 *****************************************************************************
 */
void OCS_IPN_Server::cleanup()
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_Server::cleanup()",0));

    m_threadMap.clear();
    m_ocsIPNThreadMap.clear();

    // Close listener socket
    for(int i = 0; i < 3; ++i)
    {
        if(m_listenSocket[i] > 0)
        {
            TRACE((LOG_LEVEL_INFO, "cleanup: listen socket: %d", 0, m_listenSocket[i]));

            ::shutdown(m_listenSocket[i], SHUT_RDWR);
            ::close(m_listenSocket[i]);
        }
    }
}

bool OCS_IPN_Server::bind(const int socket, const char* address, const uint16_t port)
{
    newTRACE((LOG_LEVEL_INFO, "OCS_IPN_Server::bind",0));

    sockaddr_in addr_in;
    memset(&addr_in, 0, sizeof(addr_in));
    addr_in.sin_family = AF_INET;
    addr_in.sin_addr.s_addr = inet_addr(address);
    addr_in.sin_port = htons(port);

    if (::bind(socket, (sockaddr *) &addr_in, sizeof(sockaddr_in)) < 0)
    {
        TRACE((LOG_LEVEL_ERROR, "bind() to address %s at port %d failed: %s", 0, address ,port, strerror(errno)));
        return false;
    }

    TRACE((LOG_LEVEL_INFO, "bind() to address %s at port %d successful", 0, address ,port));

    return true;
}
