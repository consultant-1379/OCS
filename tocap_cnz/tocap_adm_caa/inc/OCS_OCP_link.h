//******************************************************************************
// NAME
// OCS_OCP_link.h
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
// 2008-02-08 by EAB/FTE/DDH UABCAJN

// DESCRIPTION
// This class implements a link. A link has an IP-address and is either
// an AP, SPX or a BC link. It is also responsible for notifying the
// alarm manager about its link state.

// LATEST MODIFICATION
// -
//******************************************************************************
#ifndef _OCS_OCP_link_h
#define _OCS_OCP_link_h

#include "OCS_OCP_global.h"
#include "OCS_OCP_session.h"
#include "OCS_OCP_alarmMgr.h"

#include <vector>

class Link
{
public:
	Link(AlarmMgr* am,TOCAP::TypeOfLink tol,uint32_t ipaddr,std::string ipdotaddr,bool thisNode);
	// destructor is not needed.
	void operator()(std::ofstream& of,unsigned int& recNr);
	void addSession(const HB_Session* hbs);
	void addOtherAPlink(const Link* aplink);
	void checkStatus(bool alwaysReport=false);
	bool isLinkStable(void);
	uint32_t getIP(void) const;
	std::string getIPdot(void) const;
	TOCAP::LinkState getLinkState(void) const;
	TOCAP::TypeOfLink getLinkType(void) const;
	TOCAP::SpxState getSpxState(void);
	bool isSameNetwork(const std::string& ip) const;
	bool isLinkOnFrontEnd(void) const;
	bool isLinkStateUpdated(void) { return stateUpdated; };


private:
	Link(const Link&);
	const Link& operator=(const Link&);

	unsigned int getSessionsByState(TOCAP::SessionState ss);
	bool connectToAP(uint32_t ip,uint16_t portNr);
	AlarmMgr* aMgr;
	TOCAP::LinkState lnkState;
	TOCAP::TypeOfLink lnkType;
	uint32_t ip; // network order (big endian)
	std::string ipdot;
	std::vector<const HB_Session*> sess_vec;
	std::vector<const Link*> otherAPlinks;
	bool linkOnThisNode;
	bool stateUpdated;

	// used by the logging function to avoid logging the same data
	bool attributesUpdated; 

};


#endif
