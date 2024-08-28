//******************************************************************************
// NAME
// OCS_OCP_protHandler.cpp
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
// This class handles the protocols, i.e. the CP-AP protocol and the AP-AP
// protocol. Both protocols are defined in this class.

// LATEST MODIFICATION
// -
//******************************************************************************

#include "OCS_OCP_protHandler.h"
#include "OCS_OCP_events.h"
#include "OCS_OCP_Server.h"
#include "OCS_OCP_Trace.h"

#include <sstream>
#include <iostream>


using namespace std;


UDPSocketPtr ProtHandler::udp_socket_ptr;

//******************************************************************************
// constructor
//******************************************************************************
ProtHandler::ProtHandler(TOCAP_Events* evH)
    :sess_ptr(),
     eventRep(evH),
     bufln(0),
     bufmodifier(0),
     bufstored(0)
{
    memset(buf,0,PROT_BUFSIZE);
    memset(nodeName,0,NODENAMESIZE);
}

//******************************************************************************
// constructor
//******************************************************************************
ProtHandler::ProtHandler(TOCAP_Events* evH,const ACS_DSD_Session_Ptr& sessObjPtr) // HY34582
    :sess_ptr(sessObjPtr),
     eventRep(evH),
     bufln(0),
     bufmodifier(0),
     bufstored(0)
{
    memset(buf,0,PROT_BUFSIZE);
    memset(nodeName,0,NODENAMESIZE);
}

//******************************************************************************
// destructor
//******************************************************************************
ProtHandler::~ProtHandler()
{
    closeDSD(true);
}

//******************************************************************************
// createUDPsock
//******************************************************************************
bool ProtHandler::createUDPsock(void)
{
    try
    {
        // Create an UDP socket to transmit the status change to FrontEnd
        ProtHandler::udp_socket_ptr = boost::make_shared<UDPSocket>(); 

    }
    catch (SocketException& e)
    {
        return false;
    }

    return true;
}
//******************************************************************************
// deleteUDPsock
//******************************************************************************
void ProtHandler::deleteUDPsock(void)
{
    //delete udp_socket_ptr; //automatically call ~Socket() to close connection.
    if(udp_socket_ptr != NULL)
    {
        udp_socket_ptr->closeSocket();
    }
}
//******************************************************************************
// send_11

// Session state notification, sent by non front-end-AP node to the front-end
// node to inform about the state of the session.
//******************************************************************************
bool ProtHandler::send_11(char sstate,uint32_t apip,uint32_t cpip,uint32_t destip)
{
    bool retCode=false;
    sockaddr_in sa;

    sa.sin_port=htons(OCS_OCP_Server::s_tocapFEPort);
    sa.sin_family=AF_INET;
    sa.sin_addr.s_addr=destip;

    memcpy(&buf[0],&PRIM_11,sizeof(char));
    memcpy(&buf[INDEX_VER_1],&VER_1,sizeof(char));
    memcpy(&buf[INDEX_SESS_STATE],&sstate,sizeof(char));
    memcpy(&buf[INDEX_APIP],&apip,sizeof(uint32_t));
    memcpy(&buf[INDEX_CPIP],&cpip,sizeof(uint32_t));

    try
    {
        if(udp_socket_ptr != NULL)
        {
            udp_socket_ptr->sendto(buf, PRIM11_LENGTH, (sockaddr*)&sa, sizeof(sockaddr_in));
            retCode=true;
        }
    }
    catch (SocketException& e)
    {
        ostringstream ss;
        ss<<"send_11 failed. Message error: "<<e.what()<<". Node="<<getSessionNodeName();
        eventRep->reportEvent(10013,ss.str(),"NFE/PH");
    }


    return retCode;
}

//******************************************************************************
// send_13

// Session state notification, sent by non front-end-AP node to the front-end
// node to inform about the state of the session.
//******************************************************************************
bool ProtHandler::send_13(uint32_t apip1,uint32_t apip2,uint32_t destip)
{
    bool retCode=false;
    sockaddr_in sa;

    sa.sin_port=htons(OCS_OCP_Server::s_tocapFEPort);
    sa.sin_family=AF_INET;
    sa.sin_addr.s_addr=destip;

    memcpy(&buf[0],&PRIM_13,sizeof(char));
    memcpy(&buf[INDEX_VER_1],&VER_1,sizeof(char));
    memcpy(&buf[INDEX_APIP1],&apip1,sizeof(uint32_t));
    memcpy(&buf[INDEX_APIP2],&apip2,sizeof(uint32_t));

    try
    {
        if(udp_socket_ptr != NULL)
        {
            udp_socket_ptr->sendto(buf, PRIM13_LENGTH, (sockaddr*)&sa, sizeof(sockaddr_in));
            retCode=true;
        }
    }
    catch (SocketException& e)
    {
        ostringstream ss;
        ss<<"send_13 failed. Message error: "<<e.what()<<". Node="<<getSessionNodeName();
        eventRep->reportEvent(10013,ss.str(),"NFE/PH");
    }

    return retCode;
}


//******************************************************************************
// send_14

// Hardware table change notification, sent by front-end-AP node to the non front-end
// node to inform about the hardware table changes
//******************************************************************************
bool ProtHandler::send_14(uint32_t destip)
{
    bool retCode=false;
    sockaddr_in sa;

    sa.sin_port=htons(OCS_OCP_Server::s_tocapNFEPort);
    sa.sin_family=AF_INET;
    sa.sin_addr.s_addr=destip;

    memcpy(&buf[0],&PRIM_14,sizeof(char));
    memcpy(&buf[INDEX_VER_1],&VER_1,sizeof(char));

    try
    {
        if(udp_socket_ptr != NULL)
        {
            udp_socket_ptr->sendto(buf, PRIM14_LENGTH, (sockaddr*)&sa, sizeof(sockaddr_in));
            retCode=true;
        }
    }
    catch (SocketException& e)
    {
        ostringstream ss;
        ss<<"send_14 failed. Message error: "<<e.what()<<". Node="<<getSessionNodeName();
        eventRep->reportEvent(10013,ss.str(),"NFE/PH");
    }

    return retCode;
}

//******************************************************************************
// send_echoResp

// Echo response back to the CP.
//******************************************************************************
bool ProtHandler::send_echoResp(char respVal)
{
    bool retCode = true;

    if(sess_ptr.get() != 0)
    {
        try
        {
            memcpy(&buf[0],&respVal,sizeof(char));
            size_t sentBytes = sess_ptr->send(buf,PRIM1_LENGTH, DSD_SEND_TIMEOUT);
            retCode = (PRIM1_LENGTH == sentBytes);
        }
        catch (...)
        {
            retCode = false;
        }

        if (false == retCode)
        {
             try
             {
                 ostringstream ss;
                 ss << "send_1 failed, " << sess_ptr->last_error_text() << " rc:" << sess_ptr->last_error() <<" Node="<<getSessionNodeName();
                 eventRep->reportEvent(10013,ss.str());
             }
             catch (...)
             {
                 ostringstream ss;
                 ss << "send_1 failed, session exception Node="<<getSessionNodeName();
                 eventRep->reportEvent(10013,ss.str());
             }
        }

    }

    return retCode;
}

//******************************************************************************
// getPrimitive
//******************************************************************************
bool ProtHandler::getPrimitive(char& prim,char& ver,bool& remoteClosed)
{
    bool retCode=false;
    prim=0;
    ver=0;
    remoteClosed=false;
    bufln = 0;

    if ((retCode=loadBuffer(remoteClosed))==true)
    {
        if (bufln==0)
        {
            remoteClosed=true;
        }
        else if (bufln==PRIM1_LENGTH)
        {
            // echo primitive from CP received.
            prim=1;
        }
        else
        {
            retCode=false;
            ostringstream ss;
            ss<<"unknown primitive,length: "<<bufln<<" prim:"<<buf[0]<<" Node="<<getSessionNodeName();
            eventRep->reportEvent(10008,ss.str());
            bufln=0;
        }
    }

    return retCode;
}

//******************************************************************************
// getPrimitive_udp
//******************************************************************************
bool ProtHandler::getPrimitive_udp(char& prim,char& ver,const UDPSocketPtr& udpsocket) // HY34582
{

    bool retCode=false;
    prim=0;
    ver=0;

    // First remove possible remaining messages
    if (bufstored)
    {
        switch (buf[bufmodifier])
        {
        case (char)PRIM_11:
            bufmodifier+=PRIM11_LENGTH;
            break;
        case (char)PRIM_13:
            bufmodifier+=PRIM13_LENGTH;
            break;
        case (char)PRIM_14:
                    bufmodifier+=PRIM14_LENGTH;
                    break;
        default:
            bufstored=0;
            bufmodifier=0;
        }
    }
    // If this was the last message, reset pointers
    if ((bufstored-bufmodifier)<PRIM14_LENGTH)
    {
        bufstored=0;
        bufmodifier=0;
    }

    // If no messages are left, get new fresh ones from the net
    if (bufstored==false)
    {
        // Set timeout to 20 milliseconds
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 20000;

        udpsocket->setSockOpt(udpsocket->getSocket(),SOL_SOCKET,SO_RCVTIMEO, &tv,sizeof(tv));

        try
        {
            bufstored = udpsocket->recvfrom(buf, PROT_BUFSIZE, NULL, NULL);
        }
        catch (SocketException& e)
        {
            bufmodifier=0;
            bufstored=0;

            //ostringstream ss;
            //ss<<"Exception: " << e.what() << endl;;
            //eventRep->reportEvent(10008,ss.str());
            return false;
        }

        bufmodifier=0;
    }

    if ((buf[bufmodifier]==(char)PRIM_11) ||
        (buf[bufmodifier]==(char)PRIM_13) ||
        (buf[bufmodifier]==(char)PRIM_14))
    {
        prim=buf[bufmodifier];
        ver=buf[bufmodifier+1];
        retCode=true;
    }
    else
    {
        ostringstream ss;
        ss<<"unknown primitive_udp, length:"<<bufstored<<" prim:"<<buf[bufmodifier]<<" Node="<<getSessionNodeName();
        eventRep->reportEvent(10008,ss.str());
        bufln=0;
        bufmodifier=0;
        bufstored=0;
        retCode=false;
    }

    return retCode;
}

//******************************************************************************
// get_11
//*********************************************************************z*********
bool ProtHandler::get_11(char& sstate,uint32_t& apip,uint32_t& cpip)
{
    bool retCode=false;

    if ((bufstored-bufmodifier)>=PRIM11_LENGTH)
    {
        memcpy(&sstate,&buf[INDEX_SESS_STATE+bufmodifier],sizeof(char));
        memcpy(&apip,&buf[INDEX_APIP+bufmodifier],sizeof(uint32_t));
        memcpy(&cpip,&buf[INDEX_CPIP+bufmodifier],sizeof(uint32_t));
        retCode=true;
    }
    else
    {
        ostringstream ss;
        ss<<"get_11 failed. Node="<<getSessionNodeName();
        eventRep->reportEvent(10008,ss.str());
    }

    return retCode;
}

//******************************************************************************
// get_13
//******************************************************************************
bool ProtHandler::get_13(uint32_t& apip1,uint32_t& apip2)
{
    bool retCode=false;

    if ((bufstored-bufmodifier)>=PRIM13_LENGTH)
    {
        memcpy(&apip1,&buf[INDEX_APIP1],sizeof(uint32_t));
        memcpy(&apip2,&buf[INDEX_APIP2],sizeof(uint32_t));
        retCode=true;
    }
    else
    {
        ostringstream ss;
        ss<<"get_13 failed. Node="<<getSessionNodeName();
        eventRep->reportEvent(10008,ss.str());
    }

    return retCode;
}

//******************************************************************************
// get_echo
//******************************************************************************
bool ProtHandler::get_echo(char& echoVal)
{
    bool retCode=false;
    if (bufln==PRIM1_LENGTH)
    {
        memcpy(&echoVal,&buf[0],sizeof(char));
        if (echoVal==1 || echoVal==2 || echoVal==3 || echoVal==32)
        {
            retCode=true;
        }
        else
        {
            ostringstream ss;
            ss<<"get_echo failed, illegal echo value Node="<<getSessionNodeName();
            eventRep->reportEvent(10008,ss.str());
        }
    }
    else
    {
        ostringstream ss;
        ss<<"get_echo failed Node="<<getSessionNodeName();
        eventRep->reportEvent(10008,ss.str());
    }
    bufln=0;

    return retCode;
}

//******************************************************************************
// closeDSD
//******************************************************************************
void ProtHandler::closeDSD(bool kill)
{
        // this class is responsible for de-allocating the DSD session objects.
    if(sess_ptr.get() != 0)
    {
        try
        {
            sess_ptr->close();
            if (kill)
            {
                sess_ptr.reset();
            }

        }
        catch (...)
        {
            ostringstream ss;
            ss<<"exc. in ProtHandler destructor Node="<<getSessionNodeName();
            eventRep->reportEvent(10009,ss.str());
        }
    }
}

    
//******************************************************************************
// getSessionNodeName
//******************************************************************************
char* ProtHandler::getSessionNodeName(void)
{
   if ( 0 == nodeName[0] )
   {
       ACS_DSD_Node remNode;
       remNode.node_name[0] = 0;
       if(sess_ptr.get() != 0)
       {
           try
           {
                sess_ptr->get_remote_node(remNode);
                remNode.node_name[acs_dsd::CONFIG_NODE_NAME_SIZE_MAX - 1] = 0;
                memcpy(nodeName, remNode.node_name, acs_dsd::CONFIG_NODE_NAME_SIZE_MAX);
           }
           catch(...)
           {
               nodeName[0] = 0;
           }
       }
   }

   return nodeName;
}
//******************************************************************************
// PRIVATE CLASS METHODS
//******************************************************************************

//******************************************************************************
// loadBuffer
//******************************************************************************
bool ProtHandler::loadBuffer(bool& remoteClosed)
{
    bool retCode=true;

    if ((bufln == 0) && (sess_ptr.get() != 0))
    {
        try
        {
            bufln = PROT_BUFSIZE;

            ssize_t recBytes = sess_ptr->recv(buf,PROT_BUFSIZE, DSD_RECEIVE_TIMEOUT);

            if ( recBytes > 0 )
            {
                bufln = recBytes;
            }
            else if(recBytes == acs_dsd::ERR_PEER_CLOSED_CONNECTION)
            {
                bufln=0;
                remoteClosed = true;
            }
            else
            {
                ostringstream ss;
                ss<<"recvMsg returned false, error message: "<<sess_ptr->last_error_text()<<", error code:"<<sess_ptr->last_error()<<" Node="<<getSessionNodeName();
                eventRep->reportEvent(10014,ss.str());
                bufln=0;
                remoteClosed = true;
                retCode = false;
            }
        }
        catch (...)
        {
            ostringstream ss;
            ss << "recvMsg failed, exception thrown Node="<<getSessionNodeName();
            eventRep->reportEvent(10014,ss.str());
            bufln=0;
            remoteClosed = true;
            retCode=false;
        }
    }
    return retCode;
}

char* ProtHandler::getSessionIPAddr(void)
{

    if(sess_ptr.get() != 0)
    {
        uint32_t ip = sess_ptr->get_remote_ip4_address();
        struct in_addr in;
        in.s_addr = ip;

        return inet_ntoa(in);
    }
    else
        return 0;
}


