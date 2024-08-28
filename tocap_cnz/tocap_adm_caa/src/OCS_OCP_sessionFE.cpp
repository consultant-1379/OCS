//******************************************************************************
// NAME
// OCS_OCP_sessionFE.cpp
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
// This class supervises echo sessions on the front end (FE) AP.

// LATEST MODIFICATION
// -
//******************************************************************************

#include "OCS_OCP_sessionFE.h"
#include "OCS_OCP_link.h"
#include "OCS_OCP_Trace.h"

#include <sstream>


using namespace std;

//*************************************************************************
// constructor
//*************************************************************************
HB_Session_FE::HB_Session_FE(Link* l1,Link* l2,TOCAP_Events* ev)
:HB_Session(ev)
{
	if (l1)
	{
		links.push_back(l1);
		l1->addSession(this);
	}
	if (l2)
	{
		links.push_back(l2);
		l2->addSession(this);
	}
	time(&lastHeartBeat);
	time(&pendingStart);

	checkStatusTime = 0;

}

//******************************************************************************
// operator
//******************************************************************************
void HB_Session_FE::operator()(ofstream& of,unsigned int& recNr)
{
	if (!attributesUpdated) return;
	attributesUpdated=false;

	HB_Session::operator ()(of,recNr);
	of<<"object ref FE:"<<this<<endl;
	for (vector<Link*>::iterator i=links.begin();i!=links.end();++i)
	{
		of<<"Link object:"<<*i<<endl;
	}
	of<<"pending start:"<<pendingStart<<endl<<endl;
}

//*************************************************************************
// isObjWithIP
//*************************************************************************
bool HB_Session_FE::isObjWithIP(const uint32_t ip)
{
	bool retCode=false;
	for (vector<Link*>::iterator it=links.begin();it!=links.end();++it)
	{
      if (*it)
      {
		   if ( ip == (*it)->getIP())
		   {
			   retCode=true;
			   break;
		   }
      }
	}
	return retCode;
}

//*************************************************************************
// clockTick
//*************************************************************************
void HB_Session_FE::clockTick(void)
{
	    time_t rightNow(0);

	time(&rightNow);

	if (s_state==TOCAP::PENDING_DOWN)
	{
		if ((rightNow-pendingStart)>=(ALARM_SUPPRESSION_TIME + failoverSuppression))
		{
			attributesUpdated=true;
			s_state=TOCAP::DOWN;

			// initiate the link objects to check link status.
			// However, suppress informing the CP-link object a little more in order
			// to avoid a clash of both AP and CP alarms in certain situations.

			// index 0 is always the AP-link.
			links[0]->checkStatus();
		}
	}
	else if (s_state==TOCAP::UP || s_state==TOCAP::PENDING_UP)
	{
		if ((rightNow-lastHeartBeat)>=HB_TIMEOUT)
		{
			attributesUpdated=true;
			heartBeatTimeOut = true;
			signalSessionDown(links[1]->getIPdot());
			s_state=TOCAP::PENDING_DOWN;
			spxSide=0;
			pendingStart = rightNow;
		}
		else if (s_state==TOCAP::PENDING_UP && (rightNow-pendingStart)>=ALARM_SUPPRESSION_TIME)
		{
			attributesUpdated=true;
			s_state=TOCAP::UP;
			for (vector<Link*>::iterator it=links.begin();it!=links.end();++it)
			{
				if (*it)
				{
				   (*it)->checkStatus();
				   checkStatusTime = rightNow;
				}
			}
		}
	}
	else if (s_state==TOCAP::DOWN)
	{
		// now it's time to inform the CP link object.
        if ( pendingStart > 0 )
        {
            // index 1 is always the CP-link.
            links[1]->checkStatus();
            pendingStart = 0;
            checkStatusTime = rightNow;

        }
	}

    if (checkStatusTime > 0)
    {
        if ( !( links[0]->isLinkStateUpdated() ) )
        {
            if ( (rightNow - checkStatusTime) > HB_TIMEOUT )
            {
                links[0]->checkStatus();
                checkStatusTime = rightNow;
            }
        }
        else if ( !( links[1]->isLinkStateUpdated() ) )
        {
            if ( (rightNow - checkStatusTime) > HB_TIMEOUT )
            {
                links[1]->checkStatus();
                checkStatusTime = rightNow;
            }
        }
        else
        {
            newTRACE((LOG_LEVEL_INFO,"HB_Session_FE::clockTick",0));

            checkStatusTime = 0;

            ostringstream trace;
            if (TOCAP::UP == s_state)
            {
                trace << "State UP      " << links[0]->getIPdot().c_str() << " " << links[1]->getIPdot().c_str() ;
            }
            else if (TOCAP::DOWN == s_state)
            {
                trace << "State DOWN    " << links[0]->getIPdot().c_str() << " " << links[1]->getIPdot().c_str() ;
            }
            else
            {
                trace << "State UNKNOWN " << links[0]->getIPdot().c_str() << " " << links[1]->getIPdot().c_str() ;
            }

            TRACE((LOG_LEVEL_INFO,"%s", 0,trace.str().c_str()));

        }
	}

   this->signalSessionUp();
}

//*************************************************************************
// heartBeat
//*************************************************************************
bool HB_Session_FE::heartBeat(char latestSpxSide)
{
	time(&lastHeartBeat);
	bool cpSideSwitch=false;
	if (latestSpxSide<=3)
	{
		if (spxSide!=0 && (latestSpxSide==2 || latestSpxSide==3) && spxSide!=latestSpxSide)
		{
			cpSideSwitch=true;
		}
		spxSide=latestSpxSide;
	}

	if (s_state==TOCAP::DOWN || s_state==TOCAP::PENDING_DOWN || cpSideSwitch)
	{
		attributesUpdated=true;
		s_state=TOCAP::PENDING_UP;

		time(&pendingStart);
	}
	// reset event handle, meaning that CP-AP link is OK.
	this->heartBeatTimeOut = false;
	this->signalSessionUp();

	return true; 
	// always true, because base class needs to know if this method is implemented.
}
//*************************************************************************
// signalSessionUp
//*************************************************************************
void HB_Session_FE::signalSessionUp()
{
   HB_Session::signalSessionUp(links[1]->getIPdot());
}

//*************************************************************************
// signalSessionDisconnect
//*************************************************************************
void HB_Session_FE::signalSessionDisconnect(void)
{
	// set lastHeartBeat to zero 
	lastHeartBeat = 0; 
	clockTick();
}

//******************************************************************************
// toString
//******************************************************************************
void HB_Session_FE::toString(std::ostringstream& of)
{
	HB_Session::toString(of);
	of<<"object ref FE:"<<this<<endl;
	for (vector<Link*>::iterator i=links.begin();i!=links.end();++i)
	{
      if (*i)
      {
          of<<"Link object:"<<*i<<":"<<(*i)->getIPdot().c_str()<<endl;
	      of<<"pending start:"<<pendingStart<<endl<<endl;
      }
	}
}

