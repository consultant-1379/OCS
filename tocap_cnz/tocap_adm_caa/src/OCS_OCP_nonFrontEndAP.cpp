//******************************************************************************
// NAME
// OCS_OCP_nonFrontEndAP.cpp
//
// COPYRIGHT Ericsson AB, Sweden 2007.
// All rights reserved.
//
// The Copyright to the computer program(s) herein
// is the property of Ericsson AB, Sweden.
// The program(s) may be used and/or copied only with
// the written permission from Ericsson AB or in
// accordance with the terms and conditions stipulated in the
// agreement/contract under which the program(s) have been
// supplied.

// DOCUMENT NO
// 190 89-CAA 109 0748

// AUTHOR
// 2008-07-16 by EAB/FTE/DDH UABCAJN

// DESCRIPTION
// This is the main file for the active node on the front end AP.

// LATEST MODIFICATION
// -
//******************************************************************************
#include "OCS_OCP_nonFrontEndAP.h"

#include "OCS_OCP_global.h"
#include "OCS_OCP_session.h"
#include "OCS_OCP_sessionNFE.h"
#include "OCS_OCP_sessionFEO.h"
#include "OCS_OCP_link.h"
#include "OCS_OCP_alarmMgr.h"
#include "OCS_OCP_CSfunctions.h"
#include "OCS_OCP_events.h"
#include "OCS_OCP_protHandler.h"
#include "OCS_OCP_Trace.h"
#include "ACS_DSD_Server.h"
#include "OCS_OCP_Server.h"

#include <sstream>
#include <fstream>
#include <iostream>

using namespace std;

// Stop event for thread
Event OCS_OCP_nonFrontEndAP::s_stopEvent;
Event OCS_OCP_nonFrontEndAP::s_apNodeStateEvent;

OCS_OCP_nonFrontEndAP::OCS_OCP_nonFrontEndAP(bool multipleCP,bool frontEndSystem)
: server_CP(NULL),
  server_AP(NULL),
  m_multipleCP(multipleCP),
  m_frontEndSystem(frontEndSystem),
  m_running(false)
{
    nodeListCP.clear();
    myAP = NULL;
    echo_sess.clear();
    evRep = TOCAP_Events::getInstance();
    nodeIPaddr[0] = 0;
    nodeIPaddr[1] = 0;
    cp_handles_count = 0;
    ap_handles_count = 0;
    ap_udp_handles_count = 0;


    for(int i =0; i < MAX_NO_OF_LISTEN_HANDLES; ++i)
    {
        ap_listen_handles[i] = acs_dsd::INVALID_HANDLE;
        cp_listen_handles[i] = acs_dsd::INVALID_HANDLE;
    }

    for(int i =0; i < MAX_NO_OF_HANDLES; ++i)
    {
        dsd_session_handles[i] = acs_dsd::INVALID_HANDLE;
    }

    for(int i =0; i < NUMBER_AP_HANDLES; ++i)
    {
        ap_udp_handles[i] = acs_dsd::INVALID_HANDLE;
    }


    //Reset events
    OCS_OCP_nonFrontEndAP::s_stopEvent.resetEvent();
    OCS_OCP_nonFrontEndAP::s_apNodeStateEvent.resetEvent();
}

OCS_OCP_nonFrontEndAP::~OCS_OCP_nonFrontEndAP()
{

    for (vector<NodeData*>::iterator i1 = nodeListCP.begin();i1 != nodeListCP.end();++i1)
    {
        if (*i1) delete *i1;
    }
    nodeListCP.clear();

    if(myAP)
        delete myAP;

    for (vector<HB_Session*>::iterator i5 = echo_sess.begin();i5 != echo_sess.end();++i5)
    {
        if (*i5) delete *i5;
    }
    echo_sess.clear();

    TOCAP_Events::deleteInstance(evRep);

    if (server_CP)
    {
        server_CP->close();
        delete server_CP;
        server_CP = NULL;
    }

    //close UDPSocket
    for (int i = 0; i < NUMBER_AP_HANDLES; ++i)
    {
        if(udp_socket[i] != NULL)   // HY34582
            udp_socket[i]->closeSocket();
    }

    if (server_AP)
    {
        server_AP->close();
        delete server_AP;
        server_AP = NULL;
    }

    ProtHandler::deleteUDPsock();
}


TOCAP::RetCodeValue OCS_OCP_nonFrontEndAP::createCPAPSessions()
{

    newTRACE((LOG_LEVEL_INFO,"OCS_OCP_nonFrontEndAP::createCPAPSessions()",0));

    TOCAP::RetCodeValue rc = TOCAP::RET_OK;


    myAP = new NodeData();
    bool dummy = false; // it's value is of no interest.

    // --------------------------------------------------------------
    // Get cluster configuration data
    // --------------------------------------------------------------
    if (evRep && (rc = getCPClusterData(nodeListCP)) != TOCAP::RET_OK)
    {
        ostringstream ss;
        ss<<"getCPClusterData failed rc:"<<rc<<":";
        returnCodeValueToString(rc, ss);
        evRep->reportEvent(10000,ss.str(),"NFE/CS");
        rc = TOCAP::ANOTHER_TRY; // process won't go down, another attempt will be made.
    }
    else if (evRep && myAP && (rc = getOwnAPData(myAP,dummy)) != TOCAP::RET_OK)
    {
        ostringstream ss;
        ss<<"getOwnAPData failed rc:"<<rc<<":";
        returnCodeValueToString(rc, ss);
        ss<<" for name "<<myAP->name;
        evRep->reportEvent(10000,ss.str(),"NFE/CS");
        rc = TOCAP::ANOTHER_TRY; // process won't go down, another attempt will be made.
    }
    else if (!evRep || !myAP)
    {
        rc = TOCAP::ALLOCATION_FAILED;
    }
    else
    {
        bool notifyFrontEnd = m_multipleCP;

        if ( ( nodeListCP.size() > 4 ) && (! m_multipleCP ) )
        {
           ostringstream ss;
           ss<<"multipleCPSystem = false but Number of CP = " << (uint32_t)nodeListCP.size();
           evRep->reportEvent(10018,ss.str(),"NFE/CS");

           notifyFrontEnd = true;
        }

        // create the Session objects.
        HB_Session* tmp_hb = 0;
        for (vector<NodeData*>::iterator itcp = nodeListCP.begin();itcp != nodeListCP.end();++itcp)
        {
            if ( *itcp )
            {
                tmp_hb = new HB_Session_NFE(notifyFrontEnd,myAP->ip[0],(*itcp)->ip[0],evRep);
                if (tmp_hb) echo_sess.push_back(tmp_hb);
                tmp_hb = new HB_Session_NFE(notifyFrontEnd,myAP->ip[1],(*itcp)->ip[1],evRep);
                if (tmp_hb) echo_sess.push_back(tmp_hb);
            }
        }

        if (echo_sess.size() != (2*nodeListCP.size()))
        {
            ostringstream ss;
            ss<<"Object allocations incomplete :";
            ss<<(uint16_t)nodeListCP.size()<<":"<<(uint16_t)echo_sess.size();
            evRep->reportEvent(10001,ss.str(),"NFE/CS");
            rc = TOCAP::INCOMPLETE_CLUSTER;
        }
    }

    if (rc!=TOCAP::RET_OK)
    {
        ostringstream trace;
        trace << "mainNonFrontEndAP:: is going down RC = ";
        returnCodeValueToString(rc, trace);
        TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));

        //::sleep(1);

        return rc;
    }

    // if there are outstanding alarms issued when this node was in active state,
    // then cease them. Actually, the TOCAP on active node will ceasing all issued
    // alarm when it is stopped, but in the situation that TOCAP is stopped abnormally
    // without ceasing issued alarms.

    std::vector<NodeData*> nodeListAP;
    if (evRep && (rc = getAPClusterData(nodeListAP,myAP->name)) != TOCAP::RET_OK)
    {
        ostringstream ss;
        ss<<"getAPClusterData failed rc:"<<rc<<":";
        returnCodeValueToString(rc, ss);;
        evRep->reportEvent(10000,ss.str());
        rc = TOCAP::ANOTHER_TRY; // process won't go down, another attempt will be made.
    }

    if (rc == TOCAP::RET_OK)
    {
        AlarmMgr* tmp_a=0;

        for (vector<NodeData*>::iterator nl = nodeListAP.begin();nl != nodeListAP.end();++nl)
        {
            if (*nl)
            {
               tmp_a = new AlarmMgr((*nl)->name,evRep);
               if (tmp_a)
               {
                   delete tmp_a;
                   tmp_a = NULL;
               }
            }
        }

        for (vector<NodeData*>::iterator nl = nodeListCP.begin();nl != nodeListCP.end();++nl)
        {
            if (*nl)
            {
               tmp_a = new AlarmMgr((*nl)->name,evRep);
               if (tmp_a)
               {
                   delete tmp_a;
                   tmp_a = NULL;
               }
            }
        }

        {
            ostringstream trace;
            trace << "Number of echo sessions: " << (uint16_t)echo_sess.size();
            TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));
        }

        int i = 1;
        for (vector<HB_Session*>::iterator hbs = echo_sess.begin();hbs!=echo_sess.end();++hbs)
        {
            try
            {
               if (*hbs)
               {
                   TRACE((LOG_LEVEL_INFO," ", 0));
                   ostringstream trace;
                   trace<<"Echo sessions: "<<i++<<endl;
                   (*hbs)->toString(trace);
                   TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));
               }
            }
            catch(...)
            {}
        }

        {
            ostringstream trace;
            trace<<"End of echo sessions:"<<endl;
            trace<<"Number of CP nodes:"<<(unsigned int)nodeListCP.size();
            TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));
        }

        i = 1;
        for (vector<NodeData*>::iterator nl = nodeListCP.begin();nl != nodeListCP.end();++nl)
        {
            try
            {
               if (*nl)
               {
                    TRACE((LOG_LEVEL_INFO," ", 0));
                    ostringstream trace;
                    trace<<"Node data: "<<i++<<endl;
                    (*nl)->toString(trace);
                    TRACE((LOG_LEVEL_INFO,"%s",0, trace.str().c_str()));
               }
            }
            catch(...){}
        }

        TRACE((LOG_LEVEL_INFO, "End of CP nodes.", 0));

    }

    return rc;

}

TOCAP::RetCodeValue OCS_OCP_nonFrontEndAP::createServerObjects()
{
    newTRACE((LOG_LEVEL_INFO,"OCS_OCP_nonFrontEndAP::createServerObjects()",0));

    TOCAP::RetCodeValue rc = TOCAP::RET_OK;

    // create server objects.
    server_CP = new ACS_DSD_Server(acs_dsd::SERVICE_MODE_INET_SOCKET_PRIVATE);
    server_AP = new ACS_DSD_Server(acs_dsd::SERVICE_MODE_INET_SOCKET_PRIVATE);

    // open a server object for managing CP requests.
    ostringstream tocapCPPort, tocapNFEPort;
    tocapCPPort << OCS_OCP_Server::s_tocapCPPort;
    tocapNFEPort << OCS_OCP_Server::s_tocapNFEPort;

    if (server_CP && server_CP->open(acs_dsd::SERVICE_MODE_INET_SOCKET_PRIVATE, tocapCPPort.str()) < 0)
    {
            ostringstream trace;
            trace << "Open DSD server failed. Last error message: " << server_CP->last_error_text() ;
            TRACE((LOG_LEVEL_ERROR,"%s",0, trace.str().c_str()));

            ostringstream ss;
            ss<<server_CP->last_error_text()<<" cp open() rc:"<<server_CP->last_error();
            evRep->reportEvent(10002,ss.str(),"NFE/DSD");
            rc = TOCAP::OPEN14007_FAILED;
    }
    // open a server object for managing AP requests.
    else if (server_AP && server_AP->open(acs_dsd::SERVICE_MODE_INET_SOCKET_PRIVATE, tocapNFEPort.str()) < 0)
    {
            ostringstream trace;
            trace << "Open DSD server failed. Last error message: " << server_AP->last_error_text();
            TRACE((LOG_LEVEL_ERROR,"%s",0, trace.str().c_str()));

            ostringstream ss;
            ss<<server_AP->last_error_text()<<" cp open() rc:"<<server_AP->last_error();
            evRep->reportEvent(10002,ss.str(),"NFE/DSD");
            rc=TOCAP::OPEN14009_FAILED;
    }
    else if (server_CP && server_AP)
    {
        // get the listening handles for both servers.
        acs_dsd::HANDLE listenH[8]; //Review why not allocate memory for this pointer???
        // Initialize handles and counter variables
        int noCpListenH = 8;

        if (server_CP->get_handles(listenH, noCpListenH) >= 0)
        {
            cp_handles_count = noCpListenH;
            for (int i = 0;i < noCpListenH;i++) cp_listen_handles[i] = listenH[i];

            //listenH = 0;
            int noApListenH = 8;
            if (server_AP->get_handles(listenH, noApListenH) >= 0)
            {
                ap_handles_count = noApListenH;
                for (int i = 0;i<noApListenH;i++) ap_listen_handles[i] = listenH[i];
            }
            else
                rc = TOCAP::AP_GETHANDLES_FAILED;
        }
        else
            rc = TOCAP::CP_GETHANDLES_FAILED;

        if (rc != TOCAP::RET_OK)
        {
            ostringstream ss;
            if (rc == TOCAP::AP_GETHANDLES_FAILED)
                ss<<server_AP->last_error_text() <<" ap open() rc: " <<server_AP->last_error();
            else
                ss<<server_CP->last_error_text() <<" cp open() rc: " <<server_CP->last_error();

            evRep->reportEvent(10003,ss.str());
        }
    }
    else
    {
        ostringstream ss;
        ss<<"Allocation failure of server objects "<<server_CP<<":"<<server_AP;
        evRep->reportEvent(10004,ss.str(),"NFE/");
        rc = TOCAP::ANOTHER_TRY;
    }

    if(myAP != 0)
    {
         nodeIPaddr[0] = myAP->ip[0];
         nodeIPaddr[1] = myAP->ip[1];
    }

    // Create two UDP listening sockets for 14009 (tocapd_nfe)
    // TOCAP on frontEndAP will send the PRIM_14 to all TOCAP on nonfontEndAP to inform the HW table changes.
    int i = 0;
    try
    {
        for (i = 0; i<NUMBER_AP_HANDLES; ++i)
        {
            // Open an UDP socket for both links
            udp_socket[i] = boost::make_shared<UDPSocket>(); // HY34582

            //Set reuse address option
            int reuse = 1;

            udp_socket[i]->setSockOpt(udp_socket[i]->getSocket(), SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
            // bind the sockets to the IP Addresses on Port 14009
            udp_socket[i]->bind(nodeIPaddr[i], OCS_OCP_Server::s_tocapNFEPort);
            // Add udp socket handle to ap_udp_handles
            ap_udp_handles[i] = udp_socket[i]->getSocket();
            ap_udp_handles_count = i+1;;

        }
    }
    catch (SocketException& e)
    {
        ostringstream ss;
        ss << e.what() << " on link " << i;
        evRep->reportEvent(10015,ss.str(),"NFE/");
        if (1 == i)        // Allow failure on one link
        {
            rc = TOCAP::ANOTHER_TRY;
        }
    }

    // Create a send socket for UDP messages
    if (ProtHandler::createUDPsock()==false)
    {
        ostringstream ss;
        ss<<"Failure creating UDP socket";
        evRep->reportEvent(10015,ss.str(),"NFE/");
        rc=TOCAP::ANOTHER_TRY;
    }

    return rc;

}


TOCAP::RetCodeValue OCS_OCP_nonFrontEndAP::run() throw(SocketException)
{

    newTRACE((LOG_LEVEL_INFO,"OCS_OCP_nonFrontEndAP::run()",0));

    TOCAP::RetCodeValue rc = TOCAP::RET_OK;

    //Create CP-AP session.
    rc = createCPAPSessions();
    if(rc != TOCAP::RET_OK)
    {
        ostringstream trace;
        trace << "Failed at createCPAPSessions(). Return code: " << (uint16_t)rc << endl;
        returnCodeValueToString(rc, trace);
        TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));

        ::sleep(1);
        return rc;
    }

    //Create Server objects.
    rc  = createServerObjects();
    if(rc != TOCAP::RET_OK)
    {
        ostringstream trace;
        trace << "Failed at createServerObjects(). Return code: " << (uint16_t)rc << endl;
        returnCodeValueToString(rc, trace);
        TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));

        ::sleep(1);
        return rc;
    }

    int sockfd, maxfd, max_index;
    fd_set readset, allset;

    for (int i = 0; i < MAX_NO_OF_HANDLES; ++i)
    {
        dsd_session_handles[i] = -1; //indicate that the client is available.
    }

    //Set allset to zero
    FD_ZERO(&allset);
    max_index = -1; // index for DSD session handles array.

    //Initialize maxfd.
    maxfd = 0;

    // Add stop event to allset
    FD_SET(OCS_OCP_nonFrontEndAP::s_stopEvent.getFd(), &allset);
    if (maxfd < OCS_OCP_nonFrontEndAP::s_stopEvent.getFd())
        maxfd = OCS_OCP_nonFrontEndAP::s_stopEvent.getFd();

    // Add s_apNodeStateEvent to allset
    FD_SET(OCS_OCP_nonFrontEndAP::s_apNodeStateEvent.getFd(), &allset);
    if (maxfd < OCS_OCP_nonFrontEndAP::s_apNodeStateEvent.getFd())
        maxfd = OCS_OCP_nonFrontEndAP::s_apNodeStateEvent.getFd();


    //add CP listeners socket to select set
    for (int i = 0; i < cp_handles_count; ++i)
    {
        FD_SET(cp_listen_handles[i], &allset);
        if(maxfd < cp_listen_handles[i])
            maxfd = cp_listen_handles[i];

    }

    //add AP listeners socket to select set.
    for (int i = 0; i < ap_handles_count; ++i)
    {
        FD_SET(ap_listen_handles[i], &allset);
        if(maxfd < ap_listen_handles[i])
            maxfd = ap_listen_handles[i];

    }

    //add UDP AP sockets to select set.
    for (int i = 0; i < ap_udp_handles_count; ++i)
    {
        FD_SET(ap_udp_handles[i], &allset);
        if(maxfd < ap_udp_handles[i])
            maxfd = ap_udp_handles[i];

    }

    map<acs_dsd::HANDLE, ProtHandler_Ptr> protMap;
    map<acs_dsd::HANDLE, ProtHandler_Ptr>::iterator it_protmap;
    map<acs_dsd::HANDLE, HB_Session*> sessMap;
    map<acs_dsd::HANDLE, HB_Session*>::iterator it_sessmap;

    timeval selectTimeout;
    selectTimeout.tv_sec = 1;
    selectTimeout.tv_usec = 0;
    time_t timeNow(0);
    time(&timeNow); //return in seconds since epoch.
    time_t lastTimeout(timeNow);

    vector<map<acs_dsd::HANDLE,ProtHandler_Ptr>::iterator> removeItVec;

    int m_stop = false;
    acs_dsd::HANDLE sessionHandles[64]; //Hack
    int sessHandleCount = 64;

    ProtHandler UDP_ph(evRep);

    m_running = true;

    while(!m_stop)
    {
        //sleep 10 millisecond to prevent the loop is hung when select function is returned continuously.
        struct timespec sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_nsec = 10*1000000;
        nanosleep(&sleepTime,0);

        readset = allset;
        int select_return = ::select(maxfd+1,&readset, NULL, NULL, &selectTimeout);

        //
        selectTimeout.tv_sec = 1;
        selectTimeout.tv_usec = 0;
        time(&timeNow);

        if(select_return == 0)//timeout
        {
            lastTimeout = timeNow;

            // All session objects will be triggered to check its state, i.e
            // whether the heartbeat timeout has expired, or if the suppression
            // of a pending session state has expired.
            for (vector<HB_Session*>::iterator hbs = echo_sess.begin();hbs != echo_sess.end();++hbs)
            {
                if (*hbs)
                {
                   (*hbs)->clockTick();

                   // If hearbeat timeout, do following actions
                   // 1. Remove handle from select set
                   // 2. Remove proHandler from protMap
                   // 3. Remove hbs from sessMap

                   if((*hbs)->isHeartBeatTimeOut())
                   {
                       map<acs_dsd::HANDLE,HB_Session*>::iterator ith = sessMap.begin();
                       while (ith != sessMap.end())
                       {
                           if (ith->second == (*hbs))
                           {
                               // deallocate the protMap entry;
                               map<acs_dsd::HANDLE,ProtHandler_Ptr>::iterator itp = protMap.find(ith->first);
                               if (itp != protMap.end())
                               {
                                   //Clear this handle from select() set.
                                   {
                                       ostringstream trace;
                                       trace<<"Heartbeat timeout"<<endl;
                                       trace<<"Clear session: " <<ith->first << " from FD set.";
                                       TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));
                                   }
                                   FD_CLR(ith->first, &allset);

                                   //remove old session handle from dsd_session_handles.
                                   for (int l = 0; l < MAX_NO_OF_HANDLES; ++l)
                                   {
                                       if(dsd_session_handles[l] == ith->first)
                                       {
                                           ostringstream trace;
                                           trace<<"Remove session: " <<ith->first << " from dsd session array.";
                                           TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));

                                           dsd_session_handles[l] = -1; //make available for reuse array element.
                                           break; //Found then exit for.
                                       }
                                   }
                                   protMap.erase(itp);

                               }
                               sessMap.erase(ith);
                               break;
                           }
                           ++ith;
                       }
                   }
                }
            }
        }
        else if(select_return == -1)// error
        {
            //check errno for error message
            // handle for clearing up stuff,....

            ostringstream trace;
            trace << "Failed at select function. Error code: " << errno << endl;
            TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));

           if(errno != EINTR)// Ignore a caught signal
           {
               rc = TOCAP::SELECT_FAILED;
               break;
           }

        }
        else// handle is signed.
        {
            ostringstream trace;
            trace << "Number of handles is signaled:" << select_return <<endl;
            TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));

            /**************************************************
            1.Check for stop event
            2.Check for s_apNodeStateEvent event
            3.Check the PRIM_14 from frontEndAP
            4.Check which cp listener socket is connected.
            5.Check all ap_session_handles for data coming.
            6.Check all dsd_session_handles for data coming.
            ***************************************************/

            //1.Check for stop event
            if(FD_ISSET(OCS_OCP_nonFrontEndAP::s_stopEvent.getFd(),&readset))
            {
                // Inform front end AP that this node is going down.
                // Should only inform the frontend active node???
                vector<string> fe_names;
                if (getFrontEndNames(fe_names))
                {
                    // remove this node from fe_names, if included.
                    for (vector<string>::iterator i = fe_names.begin();i != fe_names.end();++i)
                    {
                        if (myAP->name.compare(*i)!=0)
                        {
                            vector<uint32_t> ipul_vec;
                            vector<string> ipdot_vec;
                            if (name_TO_IPaddresses(*i,ipul_vec,ipdot_vec))
                            {
                                ProtHandler ph(evRep);
                                ph.send_13(myAP->ip[0],myAP->ip[1],ipul_vec[0]);
                                ph.send_13(myAP->ip[0],myAP->ip[1],ipul_vec[1]);
                            }
                        }
                    }

                    ostringstream trace;
                    trace << "mainNonFrontEndAP:: inform front end AP that this node is going down" << endl;
                    TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));

                }

                OCS_OCP_nonFrontEndAP::s_stopEvent.resetEvent();
                m_stop = true;

                TRACE((LOG_LEVEL_INFO,"Echo_Server: receive stop event", 0));

            }

            //2.Check for s_apNodeStateEvent event
            if(FD_ISSET(OCS_OCP_nonFrontEndAP::s_apNodeStateEvent.getFd(),&readset))
            {

                OCS_OCP_nonFrontEndAP::s_apNodeStateEvent.resetEvent();

                if (m_frontEndSystem)
                {
                    rc = TOCAP::ANOTHER_TRY;
                    m_stop = true;

                    TRACE((LOG_LEVEL_INFO,"Echo_Server: receive s_apNodeStateEvent event. Node is now active :: TOCAP::INTERNAL_RESTART", 0));
                }
            }

            //3. Check the PRIM_14 from frontEndAP on UDP sockets port 14009
            for (int i = 0; i < ap_udp_handles_count; ++i)
            {
                if(ap_udp_handles[i] < 0)
                    continue;

                sockfd = ap_udp_handles[i];

                if(FD_ISSET(sockfd, &readset))
                {
                    char prim =0, ver = 0;
                    ostringstream trace;
		
		    if(udp_socket[i] == NULL)  // HY34582
                    {
                        rc = TOCAP::ANOTHER_TRY;
                        m_stop = true;
                        TRACE((LOG_LEVEL_WARN,"udp socket resource released", 0));
                        break;
                    }

                    if(UDP_ph.getPrimitive_udp(prim, ver, udp_socket[i]))
                    {
                        if(prim == PRIM_14)
                        {
                            trace<<"Received PRIM_14 from front end AP on link: " <<i <<endl;
                            TRACE((LOG_LEVEL_INFO,"%s", 0, trace.str().c_str()));

                            rc = TOCAP::ANOTHER_TRY;
                            m_stop = true;

                            TRACE((LOG_LEVEL_INFO,"TOCAP::INTERNAL_RESTART due to HW table change. NFE/",0));

                            break;
                        }
                        else
                        {
                            trace<<"Received primitive: " <<(int)prim << " from front end AP on link: " <<(int)i << endl;
                            TRACE((LOG_LEVEL_INFO,"%s", 0, trace.str().c_str()));

                        }
                    }
                    else
                    {
                        trace<<"Failed to receive primitive from front end AP on link: " <<(int)i << endl;
                        TRACE((LOG_LEVEL_INFO,"%s", 0, trace.str().c_str()));
                    }
                }
            }

            //4.Check which cp listener socket is connected.
            for (int i = 0; i < cp_handles_count; i++)
            {
                if(FD_ISSET(cp_listen_handles[i], &readset))
                {
                    //Create session for this connection
                    ACS_DSD_Session_Ptr tmp_sess = boost::make_shared<ACS_DSD_Session>();
                    if(server_CP->accept(*tmp_sess) < 0)
                    {
                        ostringstream ss;
                        ss << server_CP->last_error_text()<<":"<<server_CP->last_error();
                        TRACE((LOG_LEVEL_ERROR,"%s",0,ss.str().c_str()));

                        evRep->reportEvent(10006,ss.str());

                    }
                    else
                    {
                        ACS_DSD_Node remNode;
                        try
                        {
                            memset(remNode.node_name,0,acs_dsd::CONFIG_NODE_NAME_SIZE_MAX);
                            tmp_sess->get_remote_node(remNode);
                            remNode.node_name[acs_dsd::CONFIG_NODE_NAME_SIZE_MAX - 1] = 0;
                        }
                        catch(...)
                        {
                            remNode.node_name[0] = 0;
                            evRep->reportEvent(10009,"Exception thrown in ACS_DSD_Session::getRemoteNode()","NFE/");
                        }

                        struct in_addr in;
                        in.s_addr = tmp_sess->get_remote_ip4_address();
                        ostringstream trace;
                        trace<<"CP connected:" <<remNode.node_name << ". IP address: " <<inet_ntoa(in) ;
                        TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));

                        //sessionHandles = 0;
                        sessHandleCount = 64;
                        if(tmp_sess->get_handles(sessionHandles, sessHandleCount) < 0)
                        {
                            ostringstream trace;
                            trace << "get_handles() for session failed. Last error text: " <<  tmp_sess->last_error_text();
                            TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));
                            break;
                        }


                        TRACE((LOG_LEVEL_INFO,"sessHandleCount = %d", 0,sessHandleCount));

                        int j = 0, k = 0;
                        for (j = 0; j < sessHandleCount; ++j)
                        {
                            for (k = 0; k < MAX_NO_OF_HANDLES; ++k)
                            {
                                if(dsd_session_handles[k] < 0)
                                {
                                    ostringstream trace;
                                    trace << "clients added j = " <<  j;
                                    TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));

                                    dsd_session_handles[k] = sessionHandles[j]; //indicate that the client is available.
                                    break;
                                }
                            }

                            if(k == MAX_NO_OF_HANDLES)
                            {
                                ostringstream trace;
                                trace << "Too many clients. Refuse and close this connection: " << sessionHandles[j];
                                TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));

                                tmp_sess->close();
                                break;//exit for of session handles
                            }
                            else
                            {
                                // create a protocol handler for the session.
                                bool sessionFound = false;

                                for (vector<HB_Session*>::iterator hbs = echo_sess.begin();hbs != echo_sess.end();++hbs)
                                {
                                    if ((*hbs) && ((*hbs)->isObjWithIP(tmp_sess->get_remote_ip4_address())))
                                    {
                                        struct in_addr in;
                                        in.s_addr = tmp_sess->get_remote_ip4_address();
                                        ostringstream trace;
                                        trace<<"Echo session found for IP address: " <<inet_ntoa(in) ;
                                        TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));

                                        // note: the nodeName contains the remote IP-address.
                                        sessionFound = true;
                                        // if the sessMap already contains this HB_Session, i.e there is
                                        // already a connection, then deallocate both this map entry and
                                        // the entry in protMap.
                                        map<acs_dsd::HANDLE,HB_Session*>::iterator ith = sessMap.begin();
                                        while (ith != sessMap.end())
                                        {
                                            if (ith->second == (*hbs))
                                            {
                                                // deallocate the protMap entry;
                                                map<acs_dsd::HANDLE,ProtHandler_Ptr>::iterator itp = protMap.find(ith->first);
                                                if (itp != protMap.end())
                                                {
                                                    ostringstream trace;
                                                    trace<<"Clear session: " <<ith->first << " from FD set.";
                                                    TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));

                                                    FD_CLR(ith->first, &allset);

                                                    //remove old session handle from dsd_session_handles.
                                                    for (int l = 0; l < MAX_NO_OF_HANDLES; ++l)
                                                    {
                                                        if(dsd_session_handles[l] == ith->first)
                                                        {
                                                            ostringstream trace;
                                                            trace<<"Remove old session: " <<ith->first << " from dsd session array.";
                                                            TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));

                                                            dsd_session_handles[l] = -1; //make available for reuse array element.
                                                            break; //Found then exit for.
                                                        }
                                                    }
                                                    protMap.erase(itp);

                                                }
                                                sessMap.erase(ith);
                                                break;
                                            }
                                            ++ith;
                                        }

                                        sessMap[ sessionHandles[j]] = *hbs;
                                        break; //Found hbs then exit for.
                                    }
                                }//end for of finding hbs

                                if (!sessionFound)
                                {
                                    ostringstream ss;
                                    ss<<"Cannot find session("<<(uint16_t)echo_sess.size()<<") object for:"<<remNode.node_name;
                                    evRep->reportEvent(10005,ss.str(),"NFE/");

                                    //remove this session handle from dsd_session_handles.
                                    for (int l = 0; l < MAX_NO_OF_HANDLES; ++l)
                                    {
                                        if(dsd_session_handles[l] == sessionHandles[j])
                                        {
                                            ostringstream trace;
                                            trace<<"Remove not found session: " <<sessionHandles[j] << " from dsd session array.";
                                            TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));

                                            dsd_session_handles[l] = -1; //make available for reuse array element.
                                            break; //Found then exit for.
                                        }
                                    }
                                }
                                else
                                {
                                    //Add echo protocol to protocol map for handling session communication with CP.
                                    //Create new echo protocol
                                    ProtHandler_Ptr echo_prot = boost::make_shared<ProtHandler>(evRep, tmp_sess); 

                                    protMap.insert(std::map<acs_dsd::HANDLE, ProtHandler_Ptr> ::value_type(sessionHandles[j],echo_prot));

                                    // Add new descriptor to set
                                    ostringstream trace;
                                    trace << "Add new client socket to FD set: " << dsd_session_handles[k] ;
                                    TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));

                                    FD_SET(dsd_session_handles[k], &allset);

                                    if(dsd_session_handles[k] > maxfd)
                                        maxfd = dsd_session_handles[k];

                                    if(k > max_index)
                                        max_index = k;
                                }
                            }
                        }
                    }
                }// CP session handle is signaled.
            }//End for.

            //5.Check all ap_session_handles for data coming.
            // Review this section later.
            for (int i = 0; i < ap_handles_count; ++i)
            {
                if(ap_listen_handles[i] < 0)
                    continue;

                sockfd = ap_listen_handles[i];

                if(FD_ISSET(sockfd, &readset))
                {
                    //Create session for this connection
                    ACS_DSD_Session_Ptr tmp_sess = boost::make_shared<ACS_DSD_Session>();
                    if(server_AP->accept(*tmp_sess) < 0)
                    {
                        ostringstream ss;
                        ss << server_AP->last_error_text()<<":"<<server_AP->last_error();
                        evRep->reportEvent(10006,ss.str());

                        tmp_sess->close();
                    }
                    else
                    {
                        ACS_DSD_Node remNode;
                        try
                        {
                            memset(remNode.node_name,0,acs_dsd::CONFIG_NODE_NAME_SIZE_MAX);
                            tmp_sess->get_remote_node(remNode);
                            remNode.node_name[acs_dsd::CONFIG_NODE_NAME_SIZE_MAX - 1] = 0;
                        }
                        catch(...)
                        {
                            remNode.node_name[0] = 0;
                            evRep->reportEvent(10009,"Exception thrown in ACS_DSD_Session::getRemoteNode()","NFE/");
                        }

                        struct in_addr in;
                        in.s_addr = tmp_sess->get_remote_ip4_address();
                        ostringstream trace;
                        trace<<"AP connected:" <<remNode.node_name << ". IP address: " <<inet_ntoa(in) ;
                        TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));

                        //sessionHandles = 0;
                        sessHandleCount = 64;
                        if(tmp_sess->get_handles(sessionHandles, sessHandleCount) <= 0)
                        {
                            ostringstream trace;
                            trace << "get_handles() for session failed. Last error text: " << tmp_sess->last_error_text();
                            TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));

                            tmp_sess->close();
                            break;
                        }

                        int j = 0, k = 0;
                        for (j = 0; i < sessHandleCount; ++sessHandleCount)
                        {
                            for (k = 0; k < MAX_NO_OF_HANDLES; ++k)
                            {
                                if(dsd_session_handles[k] < 0)
                                {
                                    dsd_session_handles[k] = sessionHandles[j]; //indicate that the client is available.
                                    break;
                                }
                            }

                            if(k == MAX_NO_OF_HANDLES)
                            {
                                ostringstream trace;
                                trace << "Too many clients. Refuse and close this connection: " << sessionHandles[j] ;
                                TRACE((LOG_LEVEL_INFO, "%s",0,trace.str().c_str()));

                                tmp_sess->close();
                                break;//exit for of session handles
                            }
                            else
                            {
                                // create a protocol handler for the session.
                                //Add echo protocol to protocol map for handling session communication with CP.
                                //Create new echo protocol
                                ProtHandler_Ptr echo_prot = boost::make_shared<ProtHandler>(evRep, tmp_sess); 

                                protMap.insert(std::map<acs_dsd::HANDLE, ProtHandler_Ptr> ::value_type(sessionHandles[j],echo_prot));

                                // Add new descriptor to set
                                ostringstream trace;
                                trace << "Add new client socket to FD set: " << dsd_session_handles[k] ;
                                TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));

                                FD_SET(dsd_session_handles[k], &allset);

                                if(dsd_session_handles[k] > maxfd)
                                    maxfd = dsd_session_handles[k];

                                if(k > max_index)
                                    max_index = k;
                            }
                        }
                    }

                }//if ap handle is signed.
            }// End for.

           //6.Check all dsd_session_handles for data coming.
            for (int i = 0; i <= max_index; ++i)
            {
                if(dsd_session_handles[i] < 0)
                    continue;

                sockfd = dsd_session_handles[i];

                if(FD_ISSET(sockfd, &readset))
                {

                    // Here check all sessions individually.
                    bool remoteSideClosed = false;

                    ostringstream trace;
                    trace<<"Starting to check session:"<< sockfd ;
                    TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));

                    map<acs_dsd::HANDLE, ProtHandler_Ptr> ::iterator hp = protMap.find(sockfd);
                    if (hp != protMap.end())
                    {
                        ostringstream trace;
                        trace << "Session signaled:" << sockfd << ". CP name: " << (*hp).second->getSessionNodeName() << ". CP IP address: " << (*hp).second->getSessionIPAddr() << endl;
                        TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));

                        char primitive = 0;
                        char version = 0;

                        if ((*hp).second->getPrimitive(primitive,version,remoteSideClosed))
                        {
                            if (!remoteSideClosed)
                            {
                                ostringstream trace;
                                trace << "CP name: " << (*hp).second->getSessionNodeName() << ". CP IP address: " << (*hp).second->getSessionIPAddr() <<  ". PRIMITIVE: " << (int)primitive << " received" << endl;
                                TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));

                            }

                            if (remoteSideClosed)
                            {
                                // the map elements will be erased after this loop.
                                removeItVec.push_back(hp);
                                it_sessmap = sessMap.find(hp->first);
                                if (it_sessmap != sessMap.end())
                                {
                                    if ((*it_sessmap).second)
                                    {
                                        (*it_sessmap).second->signalSessionDisconnect();
                                    }
                                }
                            }
                            else if (primitive == PRIM_1)
                            {
                                // --------------------------
                                // ECHO primitive from CP
                                // --------------------------
                                char echoVal = 0;
                                if ((*hp).second->get_echo(echoVal))
                                {

                                    // find the HB_Session object.
                                    it_sessmap = sessMap.find(hp->first);
                                    if (it_sessmap != sessMap.end())
                                    {
                                        if ((*it_sessmap).second)
                                        {
                                            ostringstream trace;
                                            trace << "Echo value: " << (int)echoVal << endl;
                                            TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));

                                            if ((*it_sessmap).second->heartBeat(echoVal))
                                            {
                                               (*hp).second->send_echoResp(++echoVal);
                                            }
                                        }
                                    }
                                    else
                                    {
                                        // this shouldn't happen.
                                        evRep->reportEvent(10007,"cannot find HB_Session for prim 1","NFE/");
                                        // the map elements will be erase after this loop.
                                        removeItVec.push_back(hp);
                                    }
                                }
                                else
                                {
                                    removeItVec.push_back(hp);
                                }
                            }
                            else
                            {
                                // unexpected primitive.
                                evRep->reportEvent(10008,"unknown primitive","NFE/");
                                removeItVec.push_back(hp);
                            }
                        }
                        else
                        {
                            // getPrimitive failed, event is already reported.
                            // the map elements will be erase after this loop.
                            removeItVec.push_back(hp);

                        }
                    }
                }//end if FD_ISSET()

                if (timeNow - lastTimeout > 5)
                {
                    selectTimeout.tv_sec = 0;
                    selectTimeout.tv_usec = 0;
                }

                // Erase the element in the protMap map and the element in the sessMap, and clear FD_SET file descriptor if
                while (removeItVec.size())
                {
                    ostringstream trace;
                    trace << "Removing session: " << (uint16_t)removeItVec.back()->first << endl;
                    TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));

                    //Remove session from sessMap
                    map<acs_dsd::HANDLE,HB_Session*>::iterator itsess = sessMap.find(removeItVec.back()->first);
                    if (itsess != sessMap.end())
                    {
                        sessMap.erase(itsess);
                    }

                    //
                    //Clear this handle from select() set.
                    FD_CLR(removeItVec.back()->first, &allset);

                    //remove old session handle from dsd_session_handles.
                    for (int l = 0; l < MAX_NO_OF_HANDLES; ++l)
                    {
                        if(dsd_session_handles[l] == removeItVec.back()->first)
                        {
                            dsd_session_handles[l] = -1; //make available for reuse array element.
                            break; //Found then exit for.
                        }
                    }

                    //Remove from protMap
                    protMap.erase(removeItVec.back());

                    //pop
                    removeItVec.pop_back();
                }
            }
        }
    }// End while

    m_running = false;

    ::sleep(1);
    return rc;
}

void OCS_OCP_nonFrontEndAP::stop() throw(SocketException)
{
    OCS_OCP_nonFrontEndAP::s_stopEvent.setEvent();
}

bool OCS_OCP_nonFrontEndAP::isRunning()
{
    return m_running;
}

void OCS_OCP_nonFrontEndAP::setAPNodeStateEvent()
{
    OCS_OCP_nonFrontEndAP::s_apNodeStateEvent.setEvent();
}
