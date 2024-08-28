///******************************************************************************
// NAME
// OCS_OCP_HWCTableObserver.cpp
// COPYRIGHT Ericsson AB, Sweden 2014.
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
// 190 89-CAA 109 1544

// AUTHOR
// 2014-08-12 by XDT/DEK XTUNGVU

// DESCRIPTION
// This file contains class implementation to Hardware Table Observer.



#include "OCS_OCP_HWCTableObserver.h"
#include "OCS_OCP_frontEndAP.h"
#include "OCS_OCP_Trace.h"

#include <sstream>
#include <fstream>
#include <iostream>

using namespace std;

OCS_OCP_HWCTableObserver::OCS_OCP_HWCTableObserver()
{

}

OCS_OCP_HWCTableObserver::~OCS_OCP_HWCTableObserver()
{

}

// The function handles subscribing HWC Table change notification.
void OCS_OCP_HWCTableObserver::subscribeHWCTableChanges()
{
	newTRACE((LOG_LEVEL_INFO, "OCS_OCP_HWCTableObserver::subscribeHWCTableChanges()", 0));
	try
	{
		// Subscribe for changes in the HWC table
		ACS_CS_API_SubscriptionMgr* mgr = ACS_CS_API_SubscriptionMgr::getInstance();

		if (mgr)
		{
			ostringstream trace;
			ACS_CS_API_NS::CS_API_Result result = mgr->subscribeHWCTableChanges(*this);
			if (ACS_CS_API_NS::Result_Success != result)
			{
				trace<<"Fail to subscribe for changes in the HWC table. Error code: " <<result <<endl;
				TRACE((LOG_LEVEL_ERROR, "%s", 0, trace.str().c_str()));
			}
			else
			{
				TRACE((LOG_LEVEL_INFO, "Subscribed HWC table successfully.", 0 ));
			}
		}
	}
	catch (...)
	{
		ostringstream trace;
		trace<<"Exception";
		TRACE((LOG_LEVEL_ERROR, "%s", 0, trace.str().c_str()));
	}
}


// The function handles unsubscribing HWC Table change notification.
void OCS_OCP_HWCTableObserver::unsubscribeHWCTableChanges()
{
	newTRACE((LOG_LEVEL_INFO, "OCS_OCP_HWCTableObserver::unsubscribeHWCTableChanges()", 0));
	try
	{
		// Ubsubscribe for changes in the HWC table
		ACS_CS_API_SubscriptionMgr* mgr = ACS_CS_API_SubscriptionMgr::getInstance();

		if (mgr)
		{
			ostringstream trace;
			ACS_CS_API_NS::CS_API_Result result = mgr->unsubscribeHWCTableChanges(*this);
			if (ACS_CS_API_NS::Result_Success != result)
			{
				trace<<"Fail to unsubscribe for changes in the HWC table. Error code: " <<result <<endl;
				TRACE((LOG_LEVEL_ERROR, "%s", 0, trace.str().c_str()));
			}
			else
			{
				TRACE((LOG_LEVEL_INFO, "Unsubscribed HWC table successfully.", 0 ));
			}
		}
	}
	catch (...)
	{
		ostringstream trace;
		trace<<"Exception";
		TRACE((LOG_LEVEL_ERROR, "%s", 0, trace.str().c_str()));
	}
}

// Callback function when HW table changes.
void OCS_OCP_HWCTableObserver::update(const ACS_CS_API_HWCTableChange& observer)
{

	newTRACE((LOG_LEVEL_INFO, "OCS_OCP_HWCTableObserver::update()" , 0 ));
	for(int i =0; i < observer.dataSize; i++)
	{
		switch(observer.hwcData[i].operationType)
		{
		case ACS_CS_API_TableChangeOperation::Add :
		case ACS_CS_API_TableChangeOperation::Delete :
		case ACS_CS_API_TableChangeOperation::Change :
			if((observer.hwcData[i].fbn == ACS_CS_API_HWC_NS::FBN_APUB) || (observer.hwcData[i].fbn == ACS_CS_API_HWC_NS::FBN_CPUB))
			{
				TRACE((LOG_LEVEL_INFO, "HWC Table Changed, On going Internal Restart..." , 0));
				OCS_OCP_frontEndAP::setHWTableChangeEvent();
			}
			break;
		default:
			TRACE((LOG_LEVEL_INFO, "HWC Table Changed" , 0));
			break;
		}
	}
}

