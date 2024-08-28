//******************************************************************************
// NAME
// OCS_OCP_session.cpp
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
// This class is an abstract base class.
// An HB_Session object supervises the state of a CP-AP echo session. The
// object is updated by the echo session object, which sends/receives echo
// data, about the echo session status. The HB_Session object supervises
// that the echo updates arrives regularly, and if not, raises a named
// event that the particular CP port is down.

// LATEST MODIFICATION
// -
//******************************************************************************
#include "OCS_OCP_session.h"
#include "OCS_OCP_events.h"
#include <sstream>
#include <unistd.h>

using namespace std;

// static class attribute for suppressing session state changes an extra 
// 120 seconds right after a node startup.
int HB_Session::failoverSuppression=180;

//*************************************************************************
// constructor
//*************************************************************************
HB_Session::HB_Session(TOCAP_Events* ev)
:s_state(TOCAP::PENDING_DOWN),
spxSide(0),
lastHeartBeat(0),
eventHandleTime(0),
evRep(ev),
//sigHandle(NULL),
attributesUpdated(true),
heartBeatTimeOut(false)
{
}

//*************************************************************************
// destructor
//*************************************************************************
HB_Session::~HB_Session()
{
}

//******************************************************************************
// operator
//******************************************************************************
void HB_Session::operator()(ofstream& of,unsigned int& recNr)
{
	of<<"RECORD NUMBER:"<<recNr++<<endl;
	of<<"object ref:"<<this<<endl;
	if (s_state==TOCAP::PENDING_DOWN) of<<"session state: PENDING DOWN"<<endl;
	else if (s_state==TOCAP::PENDING_UP) of<<"session state: PENDING UP"<<endl;
	else if (s_state==TOCAP::DOWN) of<<"session state: DOWN"<<endl;
	else of<<"session state: UP"<<endl;
	if (spxSide==1 || spxSide==2) of<<"spx side: EX"<<endl;
	else if (spxSide==3) of<<"spx side: SB"<<endl;
	else of<<"spx side: UNKNOWN"<<endl;
	of<<"last heartbeat:"<<lastHeartBeat<<endl;
	of<<"Event object:"<<evRep<<endl;
}

//*************************************************************************
// isObjWithIP
//*************************************************************************
bool HB_Session::isObjWithIP(uint32_t,uint32_t)
{return false;}

//*************************************************************************
// isObjWithIP
//*************************************************************************
bool HB_Session::isObjWithIP(const uint32_t)
{return false;}

//*************************************************************************
// heartBeat
//*************************************************************************
bool HB_Session::heartBeat(char)
{return false;}

//*************************************************************************
// updateState
//*************************************************************************
void HB_Session::updateState(TOCAP::SessionState)
{}

//*************************************************************************
// getSessionState
//*************************************************************************
TOCAP::SessionState HB_Session::getSessionState(void) const
{
	return s_state;
}

//*************************************************************************
// getSpxState
//*************************************************************************
TOCAP::SpxState HB_Session::getSpxState(void) const
{
	if (spxSide==2 || spxSide==1) return TOCAP::EX;
	else if (spxSide==3) return TOCAP::SB;
	return TOCAP::XS_NOT_DEFINED;
}

//*************************************************************************
// getLastHeartBeat
//*************************************************************************
time_t HB_Session::getLastHeartBeat(void) const
{
	return lastHeartBeat;
}

//*************************************************************************
// signalSessionDown
//*************************************************************************
void HB_Session::signalSessionDown(std::string cpDotAddr)
{
   ::sleep(0);
   //SetEvent(sigHandle); //Be back later

   time(&eventHandleTime);
   ::sleep(0);  /// give waiting threads a go!
   ostringstream ss;
   ss<<"Echo signalling for CP address:"<<cpDotAddr.c_str()<<" down";
   if ( 0 == lastHeartBeat )
   {
      evRep->reportEvent(10017,ss.str());	
   }
   else
   {
      evRep->reportEvent(10011,ss.str());	
   }
   ::sleep(0);  /// give waiting threads a go!
}

//*************************************************************************
// signalSessionUp
//*************************************************************************
void HB_Session::signalSessionUp(std::string /*cpDotAddr*/)
{
	eventHandleTime = 0;
}

//******************************************************************************
// toString
//******************************************************************************
void HB_Session::toString(std::ostringstream& of)
{
	of<<"object ref:"<<this<<endl;
	if (s_state==TOCAP::PENDING_DOWN) of<<"session state: PENDING DOWN"<<endl;
	else if (s_state==TOCAP::PENDING_UP) of<<"session state: PENDING UP"<<endl;
	else if (s_state==TOCAP::DOWN) of<<"session state: DOWN"<<endl;
	else if (s_state==TOCAP::UNKNOWN) of<<"session state: UNKNOWN"<<endl;
	else of<<"session state: UP"<<endl;
	if (spxSide==1 || spxSide==2) of<<"spx side: EX"<<endl;
	else if (spxSide==3) of<<"spx side: SB"<<endl;
	else of<<"spx side: UNKNOWN"<<endl;
	of<<"last heartbeat:"<<lastHeartBeat<<endl;
	of<<"Event object:"<<evRep<<endl;
}

//******************************************************************************
// isHearBeatTimeOut
//******************************************************************************
bool HB_Session::isHeartBeatTimeOut()
{
    return heartBeatTimeOut;
}
