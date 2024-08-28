//******************************************************************************
// NAME
// OCS_OCP_sessionFEO.cpp
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
// 2008-02-22 by EAB/FTE/DDH UABCAJN

// DESCRIPTION
// This class supervises echo sessions which exist on a different AP, However
// the class object is instantiated by the front end AP. FEO should be
// interpreted as Front End Other(AP).

// LATEST MODIFICATION
// -
//******************************************************************************

#include "OCS_OCP_sessionFEO.h"
#include "OCS_OCP_link.h"
#include "OCS_OCP_Trace.h"

#include <sstream>


using namespace std;

//*************************************************************************
// constructor
//*************************************************************************
HB_Session_FEO::HB_Session_FEO(Link* l1,Link* l2,TOCAP_Events* ev)
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

	s_state = TOCAP::UNKNOWN;
	time(&pendingStart);
	checkStatusTime = 0;
}

//******************************************************************************
// operator
//******************************************************************************
void HB_Session_FEO::operator()(ofstream& of,unsigned int& recNr)
{
	if (!attributesUpdated) return;
	attributesUpdated=false;

	HB_Session::operator ()(of,recNr);
	of<<"object ref FEO:"<<this<<endl;
	for (vector<Link*>::iterator i=links.begin();i!=links.end();++i)
	{
		of<<"Link object:"<<*i<<endl;
	}
	of<<"pending start:"<<pendingStart<<endl<<endl;
}

//*************************************************************************
// isObjWithIP
//*************************************************************************
bool HB_Session_FEO::isObjWithIP(uint32_t ip1,uint32_t ip2)
{
	bool ip1Found=false;
	bool ip2Found=false;
	for (vector<Link*>::iterator it=links.begin();it!=links.end();++it)
	{
      if (*it)
      {
		   if (!ip1Found && ip1==(*it)->getIP())
		   {
			   ip1Found=true;
		   }
		   else if (!ip2Found && ip2==(*it)->getIP())
		   {
			   ip2Found=true;
		   }
      }
	}
	return (ip1Found && ip2Found);
}

//*************************************************************************
// isObjWithIP
// only used at AP node shutdown (non front end AP).
//*************************************************************************
bool HB_Session_FEO::isObjWithIP(const uint32_t apip)
{
	bool retCode=false;
	// check only the first link (AP link).
	if ( apip == links[0]->getIP() )
	{
		retCode=true;
	}

	return retCode;
}

//*************************************************************************
// clockTick
//*************************************************************************
void HB_Session_FEO::clockTick(void)
{

	time_t rightNow(0);
	time(&rightNow);
	if (s_state==TOCAP::PENDING_UP)
	{
		// the extra 2 seconds of suppression was added to differentiate
		// this class from the HB_Session_FE class suppression.
		// This means that session on non front ens nodes will be suppressed
		// a little longer (2 seconds).
		if ((rightNow-pendingStart)>=(ALARM_SUPPRESSION_TIME +2))
		{
			attributesUpdated=true;
			s_state=TOCAP::UP;

			// initiate the link objects to check link status.
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
	else if (s_state==TOCAP::PENDING_DOWN)
	{
		// the extra 2 seconds of suppression was added to differentiate
		// this class from the HB_Session_FE class suppression.
		// This means that session on non front ens nodes will be suppressed
		// a little longer (2 seconds).
		if ((rightNow-pendingStart)>=(ALARM_SUPPRESSION_TIME + 2 + failoverSuppression))
		{
			attributesUpdated=true;
			s_state=TOCAP::DOWN;

			// initiate the link objects to check link status.
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
    else if (TOCAP::UNKNOWN == s_state)
    {
        if ( pendingStart > 0 )
        {
            if ((rightNow-pendingStart)>=(ALARM_SUPPRESSION_TIME +2 +failoverSuppression))
            {
               // initiate the link objects to check link status.
               for (vector<Link*>::iterator it=links.begin();it!=links.end();++it)
               {
                   if (*it)
                   {
                       (*it)->checkStatus();
                   }
               }

               pendingStart = 0;
               checkStatusTime = rightNow;
            }
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
            newTRACE((LOG_LEVEL_INFO,"HB_Session_FEO::clockTick()",0));

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
}

//*************************************************************************
// updateState
//*************************************************************************
void HB_Session_FEO::updateState(TOCAP::SessionState s_otherAP)
{
	time_t rightNow(0);
   time(&rightNow);
	
   if ((s_state==TOCAP::PENDING_UP || s_state==TOCAP::UP || s_state==TOCAP::UNKNOWN) 
		&& s_otherAP==TOCAP::DOWN)
	{
		attributesUpdated=true;
		s_state=TOCAP::PENDING_DOWN;
		pendingStart=rightNow;
	}
   else if ((s_state==TOCAP::PENDING_DOWN  || s_state==TOCAP::DOWN || s_state==TOCAP::UNKNOWN) 
		&& s_otherAP==TOCAP::UP)
	{	
		attributesUpdated=true;
		s_state=TOCAP::PENDING_UP;
		pendingStart=rightNow;
	}
   else if (TOCAP::UNKNOWN == s_otherAP)
   {
		attributesUpdated=true;
      s_state = TOCAP::UNKNOWN;
		pendingStart=rightNow;
   }
}

//******************************************************************************
// toString  
//******************************************************************************
void HB_Session_FEO::toString(std::ostringstream& of)
{
	HB_Session::toString(of);
	of<<"object ref FEO:"<<this<<endl;
	for (vector<Link*>::iterator i=links.begin();i!=links.end();++i)
	{
      if (*i)
      {
		   of<<"Link object:"<<*i<<":"<<(*i)->getIPdot().c_str()<<endl;
		   of<<"pending start:"<<pendingStart<<endl<<endl;
      }
	}
}
