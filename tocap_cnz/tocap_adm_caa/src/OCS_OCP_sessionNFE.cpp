//******************************************************************************
// NAME
// OCS_OCP_sessionNFE.cpp
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
// 2008-02-28 by EAB/FTE/DDH UABCAJN

// DESCRIPTION
// This class supervises echo sessions on non front end (NFE) AP's (including
// the passive node on the front end AP system).

// LATEST MODIFICATION
// -
//******************************************************************************

//#include <ACS_DSD_DSA2.h>
#include "OCS_OCP_sessionNFE.h"
#include "OCS_OCP_protHandler.h"
#include "OCS_OCP_events.h"
#include "OCS_OCP_CSfunctions.h"
#include "OCS_OCP_Trace.h"
#include <sstream>

using namespace std;


string HB_Session_NFE::frontEndNodeName="";

//*************************************************************************
// constructor
//*************************************************************************
HB_Session_NFE::HB_Session_NFE(bool multiCP,uint32_t apip,uint32_t cpip,
							   TOCAP_Events* evH)
:HB_Session(evH),
multiCPsystem(multiCP),
ap_ip(apip),
cp_ip(cpip),
resendNotification(false)
{
	time(&lastHeartBeat);
	time(&lastNotificationSent);
}

//*************************************************************************
// destructor
//*************************************************************************
HB_Session_NFE::~HB_Session_NFE()
{
}

//******************************************************************************
// operator
//******************************************************************************
void HB_Session_NFE::operator()(ofstream& of,unsigned int& recNr)
{
	if (!attributesUpdated) return;
	attributesUpdated=false;

	HB_Session::operator ()(of,recNr);
	of<<"object ref NFE:"<<this<<endl;
	if (multiCPsystem) of<<"MULTIPLE CP SYSTEM"<<endl;
	else of<<"SINGLE CP SYSTEM"<<endl;
	of<<"AP IP address:"<<ulong_to_dotIP(ap_ip)<<endl;
	of<<"CP IP address:"<<ulong_to_dotIP(cp_ip)<<endl;
	of<<"Resend notification:"<<resendNotification<<endl;
	of<<"last notification sent:"<<lastNotificationSent<<endl;
	of<<endl;
}

//*************************************************************************
// isObjWithIP
//*************************************************************************
bool HB_Session_NFE::isObjWithIP(const uint32_t ip)
{
	bool retCode=false;

	if (ip == cp_ip)
	{
		retCode=true;
	}
	return retCode;
}

//*************************************************************************
// clockTick
//*************************************************************************
void HB_Session_NFE::clockTick(void)
{
    time_t rightNow(0);
    time(&rightNow);

	if ( s_state != TOCAP::DOWN )
	{
		if ((rightNow-lastHeartBeat)>=HB_TIMEOUT)
		{
			s_state=TOCAP::DOWN;
			attributesUpdated=true;
			heartBeatTimeOut = true;
			signalSessionDown(ulong_to_dotIP(cp_ip));
		   if (multiCPsystem) notifyFrontEndAP();
		}
	}
   
    if (multiCPsystem) 
    {
	  // there is no need to update FE in a single CP system,
	  // because the CP is responsible for alarm handling in that case.
      if ( (rightNow - lastNotificationSent) > NOTIFICATION_INTERVAL ) 
	  {
		   notifyFrontEndAP();
	  }
      else if ( resendNotification && ((rightNow - lastNotificationSent) > MIN_NOTIFICATION_INTERVAL ) )
	  {
		notifyFrontEndAP();
	  }
	}
    /// reset event
    this->signalSessionUp();
}

//*************************************************************************
// heartBeat
//*************************************************************************
bool HB_Session_NFE::heartBeat(char latestSpxSide)
{
	time(&lastHeartBeat);
	if (latestSpxSide<=3) spxSide=latestSpxSide;
	
	if ( s_state != TOCAP::UP )
	{	
		attributesUpdated=true;
		//reset signalling handle.
		s_state=TOCAP::UP;
		// there is no need to update FE in a single CP system,
		// because the CP is responsible for alarm handling in that case.
		if (multiCPsystem) notifyFrontEndAP();
	}

	/// reset event
	this->heartBeatTimeOut = false;
	this->signalSessionUp(); // Need review

	return true; 
	// always true, because base class needs to know if this method is implemented.
}

//*************************************************************************
// notifyFrontEndAP
//*************************************************************************
bool HB_Session_NFE::notifyFrontEndAP()
{
    newTRACE((LOG_LEVEL_INFO,"HB_Session_NFE::notifyFrontEndAP()",0));

	bool retCode=false;

	ostringstream ss;
	time(&lastNotificationSent);

	// inform front end AP that this node is going down.
	vector<string> fe_names;
	if (getFrontEndNames(fe_names))
	{
		checkFrontEndNames(fe_names);

		// remove this node from fe_names, if included.
		for (vector<string>::iterator i=fe_names.begin();i!=fe_names.end();++i)
		{
			vector<uint32_t> ipul_vec;
			vector<string> ipdot_vec;
			if (name_TO_IPaddresses(*i,ipul_vec,ipdot_vec))
			{
				ProtHandler ph(evRep);
				char sessState=0;
				if (s_state==TOCAP::UP) sessState=1;
				if ((retCode=ph.send_11(sessState,ap_ip,cp_ip,ipul_vec[0]))==false)
				{
					//ss<<"send_11 failed: "<<WSAGetLastError();
					evRep->reportEvent(10012,ss.str());
				}

				if ((retCode=ph.send_11(sessState,ap_ip,cp_ip,ipul_vec[1]))==false)
				{
					//ss<<"send_11 failed: "<<WSAGetLastError();
					evRep->reportEvent(10012,ss.str());
				}

				string cpIp = ulong_to_dotIP(cp_ip);
                ostringstream trace;
                if (s_state==TOCAP::UP)
                {
                   trace << "Notify Front End AP " << cpIp.c_str() << " Up";
                }
                else
                {
                   trace << "Notify Front End AP " << cpIp.c_str() << " Down";
                }
                TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));
			}
		}
	}

	if (retCode==false)
	{
		resendNotification=true;

        TRACE((LOG_LEVEL_ERROR,"HB_Session_NFE::notifyFrontEndAP() failed", 0));
	}
	else
	{
		resendNotification=false;
	}
	return retCode;

}

//*************************************************************************
// cpulong_to_dotIP
//*************************************************************************
string HB_Session_NFE::ulong_to_dotIP(uint32_t ip)
{
	ostringstream ss;
	ss<<(ip&0x000000FF)<<"."<<((ip&0x0000FF00)>>8)<<"."<<((ip&0x00FF0000)>>16)
		<<"."<<((ip&0xFF000000)>>24);
	string dotIp=ss.str();
	return dotIp;
}

//*************************************************************************
// checkFrontEndNames
//*************************************************************************
void HB_Session_NFE::checkFrontEndNames(vector<string>& fe_names)
{
	vector<string>::iterator i=fe_names.begin();
	while (i!=fe_names.end())
	{
		if (isOwnNodeName(ap_ip,*i))
		{
			fe_names.erase(i);
			break;
		}
		++i;
	}
	// if there are two possible front end nodes set the order so that the
	// cached node name becomes the first vector element.
	if (fe_names.size()==2)
	{
		if (frontEndNodeName.compare(fe_names[1])==0)
		{
			//swap the two elements.
			string tmp=fe_names[0];
			fe_names[0]=fe_names[1];
			fe_names[1]=tmp;
		}
	}

}
//*************************************************************************
// signalSessionUp
//*************************************************************************
void HB_Session_NFE::signalSessionUp()
{
   HB_Session::signalSessionUp(ulong_to_dotIP(cp_ip));
}

//*************************************************************************
// signalSessionDisconnect

//*************************************************************************
void HB_Session_NFE::signalSessionDisconnect(void)
{
	// set lastHeartBeat to zero 
	lastHeartBeat = 0; 
	clockTick();
}

//******************************************************************************
// toString
//******************************************************************************
void HB_Session_NFE::toString(std::ostringstream& of)
{

	HB_Session::toString(of);
	of<<"object ref NFE:"<<this<<endl;
	if (multiCPsystem) of<<"MULTIPLE CP SYSTEM"<<endl;
	else of<<"SINGLE CP SYSTEM"<<endl;
	of<<"AP IP address:"<<ulong_to_dotIP(ap_ip)<<endl;
	of<<"CP IP address:"<<ulong_to_dotIP(cp_ip)<<endl;
	of<<"Resend notification:"<<resendNotification<<endl;
	of<<"last notification sent:"<<lastNotificationSent<<endl;
	of<<endl;
}


