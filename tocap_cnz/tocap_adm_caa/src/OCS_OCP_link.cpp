//******************************************************************************
// NAME
// OCS_OCP_link.cpp
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
// 2008-02-14 by EAB/FTE/DDH UABCAJN

// DESCRIPTION
// This class implements a link. A link has an IP-address and is either
// an AP, SPX or a BC link. It is also responsible for notifying the
// alarm manager about its link state.

// LATEST MODIFICATION
// -
//******************************************************************************

#include "OCS_OCP_link.h"
#include "OCS_OCP_Server.h"

#include <netinet/in.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

using namespace std;

//******************************************************************************
// constructor
//******************************************************************************
Link::Link(AlarmMgr* am,TOCAP::TypeOfLink tol,uint32_t ipaddr,
			string ipdotaddr,bool thisNode)
:aMgr(am),
lnkState(TOCAP::LINK_NOT_OK_ALL_SESSIONS_DOWN),
lnkType(tol),
ip(ipaddr),
ipdot(ipdotaddr),
linkOnThisNode(thisNode),
stateUpdated(false),
attributesUpdated(true)
{
	// add this instance to the alarm manager (only multiple CP system).
	if (aMgr) aMgr->addLink(this); // pointer is 0 in a single CP system.
}

//******************************************************************************
// operator
//******************************************************************************
void Link::operator()(ofstream& of,unsigned int& recNr)
{
	if (!attributesUpdated) return;
	attributesUpdated=false;

	of<<"RECORD NUMBER:"<<recNr++<<endl;
	of<<"object ref:"<<this<<endl;
	of<<"Alarm file object:"<<aMgr<<endl;;
	for (vector<const HB_Session*>::iterator i=sess_vec.begin();i!=sess_vec.end();++i)
	{
		of<<"Session object:"<<*i<<endl;
	}
	if (lnkState==TOCAP::LINK_NOT_OK_SESSION_DOWN) of<<"LINK NOT OK SESSION DOWN"<<endl;
	else if (lnkState==TOCAP::LINK_NOT_OK_ALL_SESSIONS_DOWN) of<<"LINK NOT OK"<<endl;
   else of<<"LINK OK"<<endl;
	if (lnkType==TOCAP::AP) of<<"node type:AP"<<endl;
	else if (lnkType==TOCAP::CPB) of<<"node type:CPB"<<endl;
	else of<<"node type:SPX"<<endl;
	of<<"ip address:"<<ipdot<<endl;
	of<<"link on this node:";
	if (linkOnThisNode) of<<"YES"<<endl;
	else of<<"NO"<<endl;
	of<<"number of other AP links:"<<(unsigned int)otherAPlinks.size()<<endl;
	of<<"state updated:"<<(int)stateUpdated<<endl;
	of<<endl;
}

//******************************************************************************
// addSession
//******************************************************************************
void Link::addSession(const HB_Session* hbs)
{
	sess_vec.push_back(hbs);
}

//******************************************************************************
// addOtherAPlink
//******************************************************************************
void Link::addOtherAPlink(const Link* aplink)
{
	// only accepts links on the same network.
	if (isSameNetwork(aplink->getIPdot())) otherAPlinks.push_back(aplink);
}

//******************************************************************************
// checkStatus

// By calling this method the link object checks whether the alarm manager
// should be notified about the link state.
//******************************************************************************
void Link::checkStatus(bool alwaysReport)
{
	// notify the alarm manager that the link is NOT OK if:	 
	// (1) CP-link: any HB_sessions is DOWN.
	// (2) AP-link on this node: all HB_sessions are DOWN and all connection
	// attempts to all other AP nodes fails.
	// (3) AP-link on another AP-node: all HB_sessions are DOWN and the connection
	// attempt to the other AP node failed.
	
	// notify the alarm manager that the link is OK if: all sessions are in state is UP

	
	// update the alarm Manager about the link state.
	if (alwaysReport) stateUpdated = false;

	TOCAP::LinkState newLinkState = TOCAP::LINK_NOT_OK_SESSION_DOWN;
	bool linkIsStable = false;
    unsigned int upSessions = getSessionsByState(TOCAP::UP);
    unsigned int downSessions = getSessionsByState(TOCAP::DOWN);
    /// UNKNOWN == sessions connected to down APs -- just cannot count them
    unsigned int unknownSessions = getSessionsByState(TOCAP::UNKNOWN);  
	if (TOCAP::AP == lnkType)
	{
      downSessions = downSessions + unknownSessions;
      unknownSessions = 0;
   }

	if (sess_vec.size() == upSessions + unknownSessions)
	{
		// All sessions are up.
		linkIsStable = true;
    	newLinkState = TOCAP::LINK_OK_ALL_SESSIONS_UP;
	}
	else 
    {
		if (sess_vec.size() == downSessions + unknownSessions)
		{
			// All sessions are down.
			linkIsStable = true;
			newLinkState = TOCAP::LINK_NOT_OK_ALL_SESSIONS_DOWN;

			if (TOCAP::AP == lnkType)
			{
				bool apConnectSucceeded = false;
				if (linkOnThisNode)
				{
					for (vector<const Link*>::iterator otherAP=otherAPlinks.begin();
						otherAP!=otherAPlinks.end();++otherAP)
					{
						if (*otherAP)
						{
							   if (connectToAP((*otherAP)->getIP(),OCS_OCP_Server::s_tocapNFEPort))
							   {
								   // the AP link seems to work.
								   apConnectSucceeded=true;
								   break;
							   }
						}
					}
				}
				else
				{
					// try to connect to the other AP node.
					apConnectSucceeded=connectToAP(ip, OCS_OCP_Server::s_tocapNFEPort);
				}
				if (apConnectSucceeded) newLinkState=TOCAP::LINK_OK_ALL_SESSIONS_UP;
			}
		}
		else if ( sess_vec.size() == (upSessions + downSessions + unknownSessions) )
		{
 			// All sessions are up or down.
			linkIsStable = true;

			if (TOCAP::AP == lnkType)
			{
				/// AP links are either UP or DOWN, sessions to SPX-SB sides are (almost) always down
    			newLinkState = TOCAP::LINK_OK_ALL_SESSIONS_UP;
			}
		}
   }

	if (linkIsStable && (lnkState!=newLinkState || stateUpdated==false))
	{
		attributesUpdated=true;
		lnkState=newLinkState;

		// alarm manager (aMgr) don't exist (i.e 0) in a single CP system.
		if (aMgr) stateUpdated=aMgr->linkUpdate();
		else stateUpdated=true;
	}
}

//******************************************************************************
// isLinkStable
//******************************************************************************
bool Link::isLinkStable(void)
{
	bool retCode=false;
   unsigned int upSessions = getSessionsByState(TOCAP::UP);
   unsigned int downSessions = getSessionsByState(TOCAP::DOWN);
   /// UNKNOWN == sessions connected to down APs -- just cannot count them
   unsigned int unknownSessions = getSessionsByState(TOCAP::UNKNOWN);  
	if ( sess_vec.size() == (upSessions + downSessions + unknownSessions) )
	{
		// all sessions are UP or DOWN.
		retCode=true;
	}

	return retCode;
}

//******************************************************************************
// getIP
//******************************************************************************
uint32_t Link::getIP(void) const
{
	return ip;
}

//******************************************************************************
// getIPdot
//******************************************************************************
string Link::getIPdot(void) const
{
	return ipdot;
}

//******************************************************************************
// getLinkState
//******************************************************************************
TOCAP::LinkState Link::getLinkState(void) const
{
	return lnkState;
}

//******************************************************************************
// getLinkType
//******************************************************************************
TOCAP::TypeOfLink Link::getLinkType(void) const
{
	return lnkType;
}

//******************************************************************************
// getSpxState
//******************************************************************************
TOCAP::SpxState Link::getSpxState(void)
{
	TOCAP::SpxState spxs=TOCAP::XS_NOT_DEFINED;

	if (lnkType==TOCAP::SPX)
	{
		for (vector<const HB_Session*>::iterator it=sess_vec.begin();
			it!=sess_vec.end();++it)
		{
         if (*it)
         {
			   if ((*it)->getSessionState() == TOCAP::UP || 
				   (*it)->getSessionState() == TOCAP::PENDING_UP)
			   {
				   // FEO sessions (Front End Other) have undefined spx state.
				   TOCAP::SpxState temp_spx=(*it)->getSpxState();
				   if (temp_spx!=TOCAP::XS_NOT_DEFINED)
				   {
					   // we must cover for the situation that the spx state can
					   // differ between sessions immediately after a side switch.
					   // A newer heartbeat overrides an older one.
					   time_t temp_hb=(*it)->getLastHeartBeat();
					   time_t hbValue=0;
					   if (temp_hb>hbValue)
					   {
						   hbValue=temp_hb;
						   spxs=temp_spx;
					   }
				   }
			   }
         }
		}
	}

	return spxs;
}

//******************************************************************************
// isSameNetwork
//******************************************************************************
bool Link::isSameNetwork(const string& ip) const
{
	bool retCode=false;
	string::size_type len;
	if ((len=ip.find_last_of('.'))==ipdot.find_last_of('.'))
	{
		if (ipdot.compare(0,len,ip,0,len)==0) retCode=true;
	}
	return retCode;
}

//******************************************************************************
// isLinkOnFrontEnd
//******************************************************************************
bool Link::isLinkOnFrontEnd(void) const
{
	return linkOnThisNode;
}

//******************************************************************************
// getSessionsByState (private method)
//******************************************************************************
unsigned int Link::getSessionsByState(TOCAP::SessionState ss)
{
	unsigned int noOfSessions=0;
	for (vector<const HB_Session*>::iterator it=sess_vec.begin();it!=sess_vec.end();++it)
	{
      if (*it)
      {
		   if ((*it)->getSessionState()==ss)
		   {
			   noOfSessions++;
		   }
      }
	}

	return noOfSessions;
}

//******************************************************************************
// connectToAP (private method)
//******************************************************************************
bool Link::connectToAP(uint32_t ip,uint16_t portNr)
{
	bool retCode = false;

	int sock = -1;
	if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) > 0)
	{
		// set socket to non-blocking mode
		int flags= fcntl(sock,F_GETFL, 0 );
		fcntl(sock, F_SETFL, flags | O_NONBLOCK);

		// Get the address of the requested host
		sockaddr_in localAddr_in;
		memset(&localAddr_in, 0, sizeof(localAddr_in));
		localAddr_in.sin_family = AF_INET;
		localAddr_in.sin_addr.s_addr = ip;
		localAddr_in.sin_port = htons(portNr);

		if (connect(sock, (sockaddr *) &localAddr_in, sizeof(sockaddr_in)) < 0)
		{
			if(errno == EINPROGRESS)
			{
				fd_set readset, writeset;
				FD_ZERO(&readset);
				FD_SET(sock, &readset);
				FD_ZERO(&writeset);
				FD_SET(sock, &writeset);
				timeval selectTimeout;
				selectTimeout.tv_sec = 0;
				selectTimeout.tv_usec = 10000;
				int select_return = select(sock+1,&readset, &writeset, NULL, &selectTimeout);
				if(select_return == 0)
				{
					//cout << " select function timeout" << endl;
				}
				else if(select_return == -1)
				{
					//cout << " select function failed" << endl;
				}
				else
				{
					if(FD_ISSET(sock,&readset) || FD_ISSET(sock,&writeset))
					{
						int error;
						socklen_t len = sizeof(error);
						if(getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len) == 0)
						{
							if(error == 0)
							{
								//cout << "connect successfully:" <<strerror(errno) << endl;
								retCode = true;
							}
							else
							{
								//cout << "connect failed:" <<strerror(errno) << "error:" << error << ": " << strerror(error) << endl;
							}
						}
					}
				}
			}
			else
			{
				//cout << "connect failed: " <<strerror(errno) << endl;

			}
		}
		else
		{
			//cout << " connect ok:" <<strerror(errno) << endl;
			retCode = true;
		}

		fcntl(sock, F_SETFL, flags);

		close(sock);

	}


	return retCode;

}
