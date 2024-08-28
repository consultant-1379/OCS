///******************************************************************************
// NAME
// OCS_OCP_HWCTableObserver.h
//
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
// This file contains class declaration to Hardware Table Observer.

#ifndef OCS_OCP_HWCTABLEOBSERVER_H_
#define OCS_OCP_HWCTABLEOBSERVER_H_

#include <ACS_CS_API.h>


class OCS_OCP_HWCTableObserver: public ACS_CS_API_HWCTableObserver
{
public:
    OCS_OCP_HWCTableObserver();
    virtual ~OCS_OCP_HWCTableObserver();

	// The function handles subscribing HWC Table change notification.
	void subscribeHWCTableChanges();

	// The function handles un-subscribing HWC Table change notification.
	void unsubscribeHWCTableChanges();

    // Notification that the HWC table has been updated
    void update(const ACS_CS_API_HWCTableChange& observer );
};

#endif /* OCS_OCP_HWCTABLEOBSERVER_H_ */
