//******************************************************************************
// NAME
// OCS_OCP_frontEndAP.cpp
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
#include "OCS_OCP_frontEndAP.h"
#include "OCS_OCP_Server.h"

#include "OCS_OCP_global.h"
#include "OCS_OCP_session.h"
#include "OCS_OCP_sessionFE.h"
#include "OCS_OCP_sessionFEO.h"
#include "OCS_OCP_link.h"
#include "OCS_OCP_alarmMgr.h"
#include "OCS_OCP_CSfunctions.h"
#include "OCS_OCP_events.h"
#include "OCS_OCP_protHandler.h"
#include "ACS_DSD_Server.h"
#include "OCS_OCP_Trace.h"
#include "ACS_DSD_MacrosConstants.h"

#include <sstream>
#include <fstream>
#include <iostream>
#include <time.h>


using namespace std;

// Stop event for thread
Event OCS_OCP_frontEndAP::s_stopEvent;
// AP node state change event
Event OCS_OCP_frontEndAP::s_apNodeStateEvent;
// HW table change event
Event OCS_OCP_frontEndAP::s_hwTableChangeEvent;

OCS_OCP_frontEndAP::OCS_OCP_frontEndAP(bool multipleCP, std::string thisNodeName)
: server_CP(NULL),
  m_multipleCP(multipleCP),
  m_thisNodeName(thisNodeName),
  m_running(false)
{

    nodeList.clear();
    aMgr_vec.clear();
    apLink_vec.clear();
    cpLink_vec.clear();
    echo_sess.clear();
    evRep = TOCAP_Events::getInstance();
    nodeIPaddr[0] = 0;
    nodeIPaddr[1] = 0;
    cp_handles_count = 0;


    for(int i =0; i < MAX_NO_OF_LISTEN_HANDLES; ++i)
    {
        cp_listen_handles[i] = acs_dsd::INVALID_HANDLE;
    }

    for(int i =0; i < MAX_NO_OF_HANDLES; ++i)
    {
        dsd_session_handles[i] = acs_dsd::INVALID_HANDLE;
    }

    for(int i =0; i < NUMBER_AP_HANDLES; ++i)
    {
        ap_listen_handles[i] = acs_dsd::INVALID_HANDLE;
    }

    //Reset events
    OCS_OCP_frontEndAP::s_stopEvent.resetEvent();
    OCS_OCP_frontEndAP::s_apNodeStateEvent.resetEvent();
    OCS_OCP_frontEndAP::s_hwTableChangeEvent.resetEvent();
}

OCS_OCP_frontEndAP::~OCS_OCP_frontEndAP()
{
    for (vector<NodeData*>::iterator i1 = nodeList.begin();i1 != nodeList.end();++i1)
    {
        if (*i1) delete *i1;
    }
    nodeList.clear();

    for (vector<AlarmMgr*>::iterator i2 = aMgr_vec.begin();i2 != aMgr_vec.end();++i2)
    {
        if (*i2) delete *i2;
    }
    aMgr_vec.clear();

    vector<Link*>::iterator iL;
    for (iL = apLink_vec.begin();iL != apLink_vec.end();++iL)
    {
        if (*iL) delete *iL;
    }
    apLink_vec.clear();

    for (iL = cpLink_vec.begin();iL != cpLink_vec.end();++iL)
    {
        if (*iL) delete *iL;
    }
    cpLink_vec.clear();

    for (vector<HB_Session*>::iterator i5 = echo_sess.begin();i5 != echo_sess.end();++i5)
    {
        if (*i5) delete *i5;
    }
    echo_sess.clear();

    TOCAP_Events::deleteInstance(evRep);


    //close UDPSocket
    for (int i = 0; i < NUMBER_AP_HANDLES; ++i)
    {
        if(udp_socket[i] != NULL)  // HY34582
            udp_socket[i]->closeSocket();
    }

    if (server_CP)
    {
        server_CP->close();
        delete server_CP;
        server_CP = NULL;
    }

    ProtHandler::deleteUDPsock();
}


TOCAP::RetCodeValue OCS_OCP_frontEndAP::createCPAPSessions()
{

    newTRACE((LOG_LEVEL_INFO,"OCS_OCP_frontEndAP::createCPAPSessions()",0));

    TOCAP::RetCodeValue rc = TOCAP::RET_OK;
    // --------------------------------------------------------------
    // Get cluster configuration data
    // --------------------------------------------------------------
    if (evRep && (rc = getCPClusterData(nodeList)) != TOCAP::RET_OK)
    {
        ostringstream ss;
        ss<<"getCPClusterData failed rc:"<<rc<<":";
        returnCodeValueToString(rc, ss);
        evRep->reportEvent(10000,ss.str());
        rc = TOCAP::ANOTHER_TRY; // process won't go down, another attempt will be made.
    }
    else if (evRep && (rc = getAPClusterData(nodeList,m_thisNodeName)) != TOCAP::RET_OK)
    {
        ostringstream ss;
        ss<<"getAPClusterData failed rc:"<<rc<<":";
        returnCodeValueToString(rc, ss);;
        evRep->reportEvent(10000,ss.str());
        rc = TOCAP::ANOTHER_TRY; // process won't go down, another attempt will be made.
    }
    else if (!evRep) rc = TOCAP::ALLOCATION_FAILED;

    if (rc != TOCAP::RET_OK)
    {

        ostringstream trace;
        trace << "mainFrontEndAP:: is going down RC = ";
        returnCodeValueToString(rc, trace);
        TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));
        //::sleep(1);
        return rc;
    }


    //Add nodes to run t_tocap for testing
#ifdef TOCAP_DEBUG
    addTestingNodes();
#endif
    // --------------------------------------------------------------
    // Create and associate object instances

    // 1. create AlarmMgr objects, one instance for every node.
    // 2. create Link objects, two instances for every node.
    // 3. create HB_session objects, one for every unique CP-AP link
    //    pair.
    // --------------------------------------------------------------

    // the two alarm manager objects of an SPX must be linked.
    map<unsigned int,AlarmMgr*> spx_map;
    map<unsigned int,AlarmMgr*>::iterator itspx;
    AlarmMgr* spxPartner = 0;
    AlarmMgr* tmp_a = 0;
    Link* tmp_l1 = 0;
    Link* tmp_l2 = 0;

    nodeIPaddr[0] = 0;
    nodeIPaddr[1] = 0;


    for (vector<NodeData*>::iterator nl = nodeList.begin();nl != nodeList.end();++nl)
    {
      if (*nl)
      {
           // no alarm manager is needed in a single CP system because
           // the CP is responsible for alarm handling.
           if (m_multipleCP)
           {
               spxPartner = 0;
               // every node allocates its own alarm manager.
               if ((*nl)->nodeType == TOCAP::SPX)
               {
                   itspx = spx_map.find((*nl)->sysId);
                   if (itspx != spx_map.end())
                   {
                       spxPartner = itspx->second;
                   }
               }
               tmp_a = new AlarmMgr((*nl)->name,evRep,spxPartner);
               if (tmp_a && (*nl)->nodeType == TOCAP::SPX && spxPartner == 0)
               {
                   spx_map[(*nl)->sysId] = tmp_a;
               }
               if (tmp_a) aMgr_vec.push_back(tmp_a);
           }

           // create two link objects for the node, represented by the AlarmMgr object.
           tmp_l1 = new Link(tmp_a,(*nl)->nodeType,(*nl)->ip[0],(*nl)->ipdot[0],(*nl)->thisnode);
           tmp_l2 = new Link(tmp_a,(*nl)->nodeType,(*nl)->ip[1],(*nl)->ipdot[1],(*nl)->thisnode);
           if ((*nl)->thisnode)
           {
               nodeIPaddr[0] = (*nl)->ip[0];
               nodeIPaddr[1] = (*nl)->ip[1];
           }

           if (tmp_l1 && tmp_l2)
           {
               if ((*nl)->nodeType == TOCAP::AP)
               {
                   apLink_vec.push_back(tmp_l1);
                   apLink_vec.push_back(tmp_l2);
               }
               else
               {
                   cpLink_vec.push_back(tmp_l1);
                   cpLink_vec.push_back(tmp_l2);
               }
           }
      }
    }

    // The front End AP must be able to test the links of other AP nodes, therefor
    // link objects of this node must be loaded with all other AP link objects.
    int ownLinksUpdated = 0;
    for (vector<Link*>::iterator ownL = apLink_vec.begin();ownL != apLink_vec.end();++ownL)
    {
        if (*ownL)
        {
            if ((*ownL)->isLinkOnFrontEnd())
            {
               for (vector<Link*>::iterator apL = apLink_vec.begin();apL != apLink_vec.end();++apL)
               {
                    if (*apL)
                    {
                        if ((*apL)->isLinkOnFrontEnd() == false) (*ownL)->addOtherAPlink(*apL);
                    }
               }
               ownLinksUpdated++;
               if (ownLinksUpdated == 2) break;
            }
        }
    }

    ostringstream trace1;
    trace1<<endl<<"number of nodes:"<<(unsigned int)nodeList.size()<<endl;
    trace1<<"number of alarm mgr:"<<(unsigned int)aMgr_vec.size()<<endl;
    trace1<<"number of AP links:"<<(unsigned int)apLink_vec.size()<<endl;
    trace1<<"number of CP links:"<<(unsigned int)cpLink_vec.size()<<endl;
    TRACE((LOG_LEVEL_INFO,"%s",0,trace1.str().c_str()));

    // create the HB session objects.
    HB_Session* tmp_hb = 0;
    for (vector<Link*>::iterator ita = apLink_vec.begin();ita != apLink_vec.end();++ita)
    {
            if (*ita)
            {
           for (vector<Link*>::iterator itc = cpLink_vec.begin();itc != cpLink_vec.end();++itc)
           {
                      if (*itc)
                      {
                  if ((*itc)->isSameNetwork((*ita)->getIPdot()))
                  {
                      if ((*ita)->isLinkOnFrontEnd())
                      {
                          tmp_hb = new HB_Session_FE(*ita,*itc,evRep);
                      }
                      else
                      {
                          tmp_hb = new HB_Session_FEO(*ita,*itc,evRep);
                      }
                      if (tmp_hb) echo_sess.push_back(tmp_hb);
                  }
                      }
           }
            }
    }


    ostringstream trace2;
    trace2<<"Number of echo sessions:"<<(uint16_t)echo_sess.size();
    TRACE((LOG_LEVEL_INFO, "%s",0,trace2.str().c_str()));


    int i = 1;
    for (vector<HB_Session*>::iterator hbs = echo_sess.begin();hbs != echo_sess.end();++hbs)
    {
        try
        {
            if (*hbs)
            {
                TRACE((LOG_LEVEL_INFO," \n",0));
                ostringstream trace3;
                trace3<<"Echo sessions: "<<i++<<endl;
                (*hbs)->toString(trace3);
                TRACE((LOG_LEVEL_INFO,"%s\n",0,trace3.str().c_str()));
            }

        }
        catch(...)
        {

        }
    }

    {
        ostringstream trace4;
        trace4<<"End of echo sessions:"<<endl;
        trace4<<"Number of nodes:"<<(uint16_t)nodeList.size();
        TRACE((LOG_LEVEL_INFO, "%s",0,trace4.str().c_str()));
    }

    i = 1;
    for (vector<NodeData*>::iterator nl = nodeList.begin();nl != nodeList.end();++nl)
    {
        try
        {
            if (*nl)
            {
                TRACE((LOG_LEVEL_INFO," ",0));
                ostringstream trace;
                trace<<"Node data: "<<i++<<endl;
                (*nl)->toString(trace);
                TRACE((LOG_LEVEL_INFO,"%s",0, trace.str().c_str()));
            }
        }
        catch(...){}
    }

    TRACE((LOG_LEVEL_INFO, "End of nodes:",0));

    // verify that the number of created objects are correct.
    // Link = 2*NodeData
    // HB_Session = 4 * ap_link * cp_link

    if (((apLink_vec.size()+cpLink_vec.size()) != 2*nodeList.size()) ||
        echo_sess.size() != ((apLink_vec.size()*cpLink_vec.size())/2))
    {
        ostringstream ss;
        ss<<"Object allocations incomplete :";
        ss<<(unsigned int)nodeList.size()<<":"<<(unsigned int)aMgr_vec.size()<<":"<<(unsigned int)apLink_vec.size();
        ss<<":"<<(unsigned int)cpLink_vec.size()<<":"<<(unsigned int)echo_sess.size();
        evRep->reportEvent(10001,ss.str());

        ostringstream trace6;
        trace6 << "mainFrontEndAP:: is going down RC = ";
        returnCodeValueToString(rc, trace6);
        TRACE((LOG_LEVEL_INFO,"%s",0,trace6.str().c_str()));

        //::sleep(10);
        return TOCAP::INCOMPLETE_CLUSTER;
    }

    return rc;

}

TOCAP::RetCodeValue OCS_OCP_frontEndAP::createServerObjects()
{
    TOCAP::RetCodeValue rc = TOCAP::RET_OK;

    // create server objects.
    server_CP = new ACS_DSD_Server(acs_dsd::SERVICE_MODE_INET_SOCKET_PRIVATE);

    rc = TOCAP::RET_OK;
    int i = 0;
    // Create two UDP listening sockets for 14008 (tocapd_fe)
    try
    {
        for (i = 0; i<NUMBER_AP_HANDLES; ++i)
        {
            // Open an UDP socket for both links
            udp_socket[i] = boost::make_shared<UDPSocket>();  // HY34582

            //Set reuse address option
            int reuse = 1;

            udp_socket[i]->setSockOpt(udp_socket[i]->getSocket(), SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
            // bind the sockets to the IP Addresses on Port 14008
            udp_socket[i]->bind(nodeIPaddr[i], OCS_OCP_Server::s_tocapFEPort);
            // Add udp socket handle to ap listen handles
            ap_listen_handles[i] = udp_socket[i]->getSocket();;

        }
    }
    catch (SocketException& e)
    {
        ostringstream ss;
        ss << e.what() << " on link " << i;
        evRep->reportEvent(10015,ss.str());
        if (1 == i)        // Allow failure on one link
        {
            rc = TOCAP::ANOTHER_TRY;
        }
    }

    if (rc == TOCAP::RET_OK)
    {
        // open a server object for managing CP requests.
        ostringstream tocapCPPort;
        tocapCPPort << OCS_OCP_Server::s_tocapCPPort;

        if (server_CP && server_CP->open(acs_dsd::SERVICE_MODE_INET_SOCKET_PRIVATE, tocapCPPort.str()) < 0)
        {
                ostringstream ss;
                ss<<server_CP->last_error_text()<<" cp open() rc:"<<server_CP->last_error();
                evRep->reportEvent(10002,ss.str());
                rc = TOCAP::OPEN14007_FAILED;
        }
        else
        {
            // get the listening handles for both servers.
            acs_dsd::HANDLE listenH[8];
            // Initialize handles and counter variables
            int noCpListenH = 8;

            int getHandlesResult = server_CP->get_handles(listenH, noCpListenH);

            if (getHandlesResult >= 0)
            {
                cp_handles_count = noCpListenH;
                for (int i = 0;i<noCpListenH;++i) cp_listen_handles[i] = listenH[i];
            }
            else
            {
                rc = TOCAP::CP_GETHANDLES_FAILED;
                ostringstream ss;
                ss<<server_CP->last_error_text() <<" cp open() rc: " <<server_CP->last_error();
                evRep->reportEvent(10003,ss.str());
            }
        }
    }


    // Create a send socket for UDP messages
    if (ProtHandler::createUDPsock()==false)
    {
        ostringstream ss;
        ss<<"Failure creating UDP socket";
        evRep->reportEvent(10015,ss.str(),"FE/");
        rc=TOCAP::ANOTHER_TRY;
    }

    return rc;

}

TOCAP::RetCodeValue OCS_OCP_frontEndAP::run() throw(SocketException)
{
    newTRACE((LOG_LEVEL_INFO, "OCS_OCP_frontEndAP::run()",0));

    TOCAP::RetCodeValue rc = TOCAP::RET_OK;

    //Create CP-AP session.
    rc = createCPAPSessions();
    if(rc != TOCAP::RET_OK)
    {
        ostringstream trace;
        trace << "Failed at createCPAPSessions(). Return code: "<< (uint16_t)rc << ":";
        returnCodeValueToString(rc, trace);
        TRACE((LOG_LEVEL_INFO,"%s",0,trace.str().c_str()));

        ::sleep(1);
        return rc;
    }

    //Create Server objects.
    rc  = createServerObjects();
    if(rc != TOCAP::RET_OK)
    {
        ostringstream trace;

        trace<<"Failed at createServerObjects(). Return code: " <<(uint16_t)rc << ":";
        returnCodeValueToString(rc, trace);
        TRACE((LOG_LEVEL_INFO, "%s",0,trace.str().c_str()));

        ::sleep(1);
        return rc;
    }


    int sockfd, maxfd, max_index;
    fd_set readset, allset;

    for (int i = 0; i < MAX_NO_OF_HANDLES; i++)
    {
        dsd_session_handles[i] = -1; //indicate that the client is available.
    }

    //Set allset to zero
    FD_ZERO(&allset);
    max_index = -1; // index for DSD session handles array.

    //Initialize maxfd.
    maxfd = 0;

    // Add stop event to allset
    FD_SET(OCS_OCP_frontEndAP::s_stopEvent.getFd(), &allset);
    if (maxfd < OCS_OCP_frontEndAP::s_stopEvent.getFd())
        maxfd = OCS_OCP_frontEndAP::s_stopEvent.getFd();

    // Add s_apNodeStateEvent to allset
    FD_SET(OCS_OCP_frontEndAP::s_apNodeStateEvent.getFd(), &allset);
    if (maxfd < OCS_OCP_frontEndAP::s_apNodeStateEvent.getFd())
        maxfd = OCS_OCP_frontEndAP::s_apNodeStateEvent.getFd();

    // Add s_hwTableChangeEvent to allset
    FD_SET(OCS_OCP_frontEndAP::s_hwTableChangeEvent.getFd(), &allset);
    if (maxfd < OCS_OCP_frontEndAP::s_hwTableChangeEvent.getFd())
        maxfd = OCS_OCP_frontEndAP::s_hwTableChangeEvent.getFd();

    //add CP listeners socket to select set
    for (int i = 0; i < cp_handles_count; ++i)
    {
        FD_SET(cp_listen_handles[i], &allset);
        if(maxfd < cp_listen_handles[i])
            maxfd = cp_listen_handles[i];

    }

    /*
    //add AP listeners socket to select set.
    for (int i = 0; i < NUMBER_AP_HANDLES; ++i)
    {
        FD_SET(ap_listen_handles[i], &allset);
        if(maxfd < ap_listen_handles[i])
            maxfd = ap_listen_handles[i];

    }
    */

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
    time_t lastTimeoutLinkCheck(timeNow);
    time_t timeSinceProcessStart(timeNow);
    vector<map<acs_dsd::HANDLE,ProtHandler_Ptr>::iterator> removeItVec;

    ProtHandler UDP_ph(evRep);

    int m_stop = false;
    acs_dsd::HANDLE sessionHandles[64];  // hack
    int sessHandleCount = 64;            // hack

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

        selectTimeout.tv_sec = 1;
        selectTimeout.tv_usec = 0;
        time(&timeNow);

        if(select_return == 0)//timeout
        {
            //Handle for select function timeout.
            //Handle fro AP-AP communication. Will be visit later for the next sprint (4-8)
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
                                    ostringstream trace;
                                    trace<<"Heartbeat timeout"<<endl;
                                    trace<<"Clear session: " <<ith->first << " from FD set.";
                                    TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));

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

            //Need review...
            if (difftime(lastTimeout, timeSinceProcessStart) > 300)
            {
                HB_Session::failoverSuppression = 0;
            }

            // initiate link checks regularly when there are no OCAD-TOCAP connections.
            // Faulty AP links might have been fixed.
            if ( protMap.size() == 0 )
            {
                //only initiate link checks in 60s time interval.
                if ( (lastTimeout - lastTimeoutLinkCheck) > 60)
                {
                    lastTimeoutLinkCheck = lastTimeout;
                    for (vector<Link*>::iterator iva = apLink_vec.begin();iva != apLink_vec.end();++iva)
                    {
                        if (*iva)
                        {
                            // AP links.
                            (*iva)->checkStatus();
                    }
                    }
                    for (vector<Link*>::iterator ivc = cpLink_vec.begin();ivc != cpLink_vec.end();++ivc)
                    {
                        if (*ivc)
                        {
                            // CP links.
                            (*ivc)->checkStatus(true);
                        }
                    }
                }
            }
            else
            {
                lastTimeoutLinkCheck = lastTimeout;
            }

            // Check if data has come from another AP node
            char primitive = 0;
            char version = 0;
            char s_state = 0;
            uint32_t apip = 0;
            uint32_t cpip = 0;
            for (int i = 0; i < NUMBER_AP_HANDLES; ++i)
            {
                try
                {
                    while ((udp_socket[i] != NULL) && UDP_ph.getPrimitive_udp(primitive,version,udp_socket[i]))  // HY34582
                    {

                        bool traced(false);

                        if (primitive == PRIM_11)
                        {
                            // ---------------------------------------------------------
                            // non front end AP node sending link change status message.
                            // ---------------------------------------------------------
                            if (UDP_ph.get_11(s_state,apip,cpip))
                            {
                                // find correct HB_Session object which corresponds to the IP-addresses.
                                vector<HB_Session*>::iterator is = echo_sess.begin();
                                try
                                {
                                    while( is != echo_sess.end() && ( (NULL == (*is)) || ( (*is)->isObjWithIP(apip,cpip) == false )))
                                    {
                                       ++is;
                                    }
                                }
                                catch(...)
                                {
                                    is = echo_sess.end();
                                }

                                string apIpAddress = netOrderIP_TO_dotIP(apip);
                                string cpIpAddress = netOrderIP_TO_dotIP(cpip);

                                if (is != echo_sess.end())
                                {
                                    TOCAP::SessionState sessState = TOCAP::UP;
                                    if (s_state == 0) sessState = TOCAP::DOWN;

                                    (*is)->updateState(sessState);

                                    traced = true;
                                    ostringstream trace;
                                    trace<<"UDP["<<i<<"] Received PRIM_11 from AP link:"<<apIpAddress.c_str()<<endl;
                                    trace<<"CP link:"<<cpIpAddress.c_str()<<(( TOCAP::UP == sessState ) ? " UP" : " DOWN" ) ;
                                    TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));

                                }
                                else
                                {
                                    traced = true;
                                    ostringstream tmp;
                                    tmp << "cannot find HB_Session for prim 11 with AP node: " << apIpAddress.c_str() << " and CP node: " << cpIpAddress.c_str();
                                    evRep->reportEvent(10007, tmp.str());
                                }
                            }
                        }
                        else if (primitive == PRIM_13)
                        {
                            // ----------------------------------------------
                            // non front end AP node down.
                            // ----------------------------------------------
                            uint32_t apip1 = 0;
                            uint32_t apip2 = 0;
                            if (UDP_ph.get_13(apip1,apip2))
                            {
                                for (vector<HB_Session*>::iterator hbs = echo_sess.begin(); hbs != echo_sess.end();++hbs)
                                {
                                    if (*hbs)
                                    {
                                        if ((*hbs)->isObjWithIP(apip1))
                                        {
                                           (*hbs)->updateState(TOCAP::UNKNOWN);
                                        }
                                        if ((*hbs)->isObjWithIP(apip2))
                                        {
                                           (*hbs)->updateState(TOCAP::UNKNOWN);
                                        }
                                    }
                                }

                                string ip1 = netOrderIP_TO_dotIP(apip1);
                                string ip2 = netOrderIP_TO_dotIP(apip2);
                                traced = true;
                                ostringstream trace;
                                trace<<"UDP["<<i<<"] Received PRIM_13 AP link1:"<<ip1.c_str()<<" AP link2:"<<ip2.c_str();
                                TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));
                            }
                        }

                        ostringstream trace;
                        trace<<"UDP["<<i<<"] Received data from other AP node:"<<(int)primitive;
                        TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));

                    }// while (more)

		    if(udp_socket[i] == NULL) // HY34582
                    {
                        rc = TOCAP::ANOTHER_TRY;
                        m_stop = true;
                        TRACE((LOG_LEVEL_WARN,"udp socket resource released", 0));
                        break;
                    }
                }
                catch(SocketException &e)
                {
                    ostringstream trace;
                    trace<<"Exception:" << e.what() ;
                    TRACE((LOG_LEVEL_ERROR,"%s", 0,trace.str().c_str()));
                }
                catch(...)
                {
                    ostringstream trace;
                    trace<<"Exception:" ;
                    TRACE((LOG_LEVEL_ERROR,"%s", 0,trace.str().c_str()));
                }
            }    // for (i)
        }
        else if(select_return == -1)// error
        {
            //check errno for error message
            // handle for clearing up stuff,....
            ostringstream trace;
            trace<<"Failed at select function. Error code: " << errno ;
            TRACE((LOG_LEVEL_ERROR, "%s",0,trace.str().c_str()));

            if(errno != EINTR)// Ignore a caught signal
            {
               rc = TOCAP::SELECT_FAILED;
               break;
            }
        }
        else //Handle is signed.
        {

            ostringstream trace;
            trace << "Number of handles is signaled:" << select_return <<endl;
            TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));

            /**************************************************
            1.Check for stop event
            2.Check for s_apNodeStateEvent event
            3.Check for s_hwTableChangeEvent event
            4.Check which cp listener socket is connected.
            5.Check all ap_session_handles for data coming.
            6.Check all dsd_session_handles for data coming.
            ***************************************************/

            //1.Check for stop event
            if(FD_ISSET(OCS_OCP_frontEndAP::s_stopEvent.getFd(),&readset))
            {
                OCS_OCP_frontEndAP::s_stopEvent.resetEvent();
                m_stop = true;

                ostringstream trace;
                trace<<"Echo_Server: receive stop event" ;
                TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));
            }

            //2.Check for s_apNodeStateEvent event
            if(FD_ISSET(OCS_OCP_frontEndAP::s_apNodeStateEvent.getFd(),&readset))
            {
                OCS_OCP_frontEndAP::s_apNodeStateEvent.resetEvent();
                rc = TOCAP::ANOTHER_TRY;
                m_stop = true;

                TRACE((LOG_LEVEL_INFO,"Echo_Server: receive s_apNodeStateEvent event. Node is now passive :: TOCAP::INTERNAL_RESTART",0));
            }

            //3.Check for s_hwTableChangeEvent event
            if(FD_ISSET(OCS_OCP_frontEndAP::s_hwTableChangeEvent.getFd(),&readset))
            {
                OCS_OCP_frontEndAP::s_hwTableChangeEvent.resetEvent();

                // Send PRIM_14 to TOCAP on non font end APs
                for (vector<Link*>::iterator apL = apLink_vec.begin();apL != apLink_vec.end();++apL)
                {
                    if (*apL)
                    {
                        if ((*apL)->isLinkOnFrontEnd() == false)
                        {
                            ostringstream trace;
                            trace<<"Send PRIM_14 to other AP on link: " << (*apL)->getIPdot();
                            TRACE((LOG_LEVEL_ERROR, "%s",0,trace.str().c_str()));

                            UDP_ph.send_14((*apL)->getIP());
                        }
                    }
                }

                rc = TOCAP::ANOTHER_TRY;
                m_stop = true;

                TRACE((LOG_LEVEL_INFO,"TOCAP::INTERNAL_RESTART due to HW table change. FE/",0));
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
                        TRACE((LOG_LEVEL_INFO,"%s",0,ss.str().c_str()));

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
                            ostringstream ss;
                            ss<<"Exception thrown in ACS_DSD_Session::get_remote_node()";
                            evRep->reportEvent(10009,ss.str());
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
                            trace<< "get_handles() for session failed. Last error text: " << tmp_sess->last_error_text() ;
                            TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));

                            break;
                        }

                        TRACE((LOG_LEVEL_INFO,"get_handles() for session : sessHandleCount = %d", 0,sessHandleCount));

                        int j = 0, k = 0;
                        for (j = 0; j < sessHandleCount; ++j)
                        {
                            for (k = 0; k < MAX_NO_OF_HANDLES; ++k)
                            {
                                if(dsd_session_handles[k] < 0)
                                {
                                    ostringstream trace;
                                    trace << "clients added j = " <<  j ;
                                    TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));

                                    dsd_session_handles[k] = sessionHandles[j]; //indicate that the client is available.
                                    break;
                                }
                            }

                            if(k == MAX_NO_OF_HANDLES)
                            {
                                ostringstream trace;
                                trace << "Too many clients. Refuse and close this connection: " <<  sessionHandles[j] ;
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
                                                    //Clear this handle from select() set.
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
                                    ss<<"Cannot find session object for:"<<remNode.node_name;
                                    evRep->reportEvent(10005,ss.str());

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
                }// CP session handle is signalled.
            }//End for.

            /*
            //5.Check all ap_session_handles for data coming.
            // Review this section later.
            for (int i = 0; i < NUMBER_AP_HANDLES; ++i)
            {
                if(ap_listen_handles[i] < 0)
                    continue;

                sockfd = ap_listen_handles[i];

                if(FD_ISSET(sockfd, &readset))
                {
                    if (timeNow - lastTimeout > 5)
                    {
                        selectTimeout.tv_sec = 0;
                        selectTimeout.tv_usec = 0;
                    }

                    //testing
                    evRep->reportEvent(10007,"AP handle is signal");
                }
            }
            */


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
                        trace << "Session signalled:" << sockfd << ". CP name: " << (*hp).second->getSessionNodeName() << ". CP IP address: " << (*hp).second->getSessionIPAddr() ;
                        TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));

                        char primitive = 0;
                        char version = 0;
                        if ((*hp).second->getPrimitive(primitive,version,remoteSideClosed))
                        {
                            if (!remoteSideClosed)
                            {
                                ostringstream trace;
                                trace << "CP name: " << (*hp).second->getSessionNodeName() << ". CP IP address: " << (*hp).second->getSessionIPAddr() <<  ". PRIMITIVE: " << (int)primitive << " received" ;
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
                                            trace << "echo value: " << (int)echoVal ;
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
                                        TRACE((LOG_LEVEL_INFO,"cannot find HB_Session for prim 1",0));

                                        evRep->reportEvent(10007,"cannot find HB_Session for prim 1");

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
                                removeItVec.push_back(hp);
                            }
                        }
                        else
                        {
                            // getPrimitive failed, event is already reported.
                            // the map elements will be erase after this loop.
                            removeItVec.push_back(hp);
                            /*
                            if ( !remoteSideClosed )
                            {
                                //Timeout read???
                                rc = TOCAP::TIMEOUT_READ;
                                m_stop = true;
                            }
                            */
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
                    trace << "Removing session: " << (uint16_t)removeItVec.back()->first ;
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

void OCS_OCP_frontEndAP::stop() throw(SocketException)
{
    OCS_OCP_frontEndAP::s_stopEvent.setEvent();
}

bool OCS_OCP_frontEndAP::isRunning()
{
    return m_running;
}

void OCS_OCP_frontEndAP::addTestingNodes()
{
    //AP1B
    NodeData* AP1B = new NodeData();
    AP1B->name = "AP1B";
    //cp1A->nodeType = TOCAP::SPX;
    //ownAP->thisnode = false;

    AP1B->ipdot[0] = "192.168.169.2";
    AP1B->ipdot[1] = "192.168.170.2";
    AP1B->ip[0] = inet_addr(AP1B->ipdot[0].c_str());
    AP1B->ip[1] = inet_addr(AP1B->ipdot[1].c_str());
    AP1B->nodeType = TOCAP::CPB;
    this->nodeList.push_back(AP1B);

}

void OCS_OCP_frontEndAP::setAPNodeStateEvent()
{
    OCS_OCP_frontEndAP::s_apNodeStateEvent.setEvent();
}

void OCS_OCP_frontEndAP::setHWTableChangeEvent()
{
    OCS_OCP_frontEndAP::s_hwTableChangeEvent.setEvent();
}
