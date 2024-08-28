//******************************************************************************
// NAME
// OCS_OCP_alarmMgr.h
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
// 2007-12-14 by EAB/FTE/DDH UABCAJN

// DESCRIPTION
// This class decides whether and which alarm that should be issued/ceased
// depending on the node's link states.

// LATEST MODIFICATION
// -
//******************************************************************************
#ifndef _OCS_OCP_alarmMgr_h
#define _OCS_OCP_alarmMgr_h

#include <vector>
#include <fstream>
#include "OCS_OCP_global.h"
#include "OCS_OCP_alarmFile.h"
#include "OCS_OCP_events.h"

class Link;       /// forward declaration 
class AlarmMgr
{
public:
	AlarmMgr(std::string nName,TOCAP_Events* tev,AlarmMgr* spxptr=0);
	~AlarmMgr();
	void operator()(std::ofstream& of,uint32_t& recNr);
	void addLink(Link* lnk);
	bool linkUpdate(void); // link state has been updated.


private:
	AlarmMgr(const AlarmMgr&);
	const AlarmMgr& operator=(const AlarmMgr&);

	static bool isAPlinksFaulty(uint32_t network);
	static void ceaseAllCPlink(uint32_t network);
	void ceaseCPlink(uint32_t network);
	bool isAP(void); // is this object managing an AP node ?
	bool isAPlinkFaulty(uint32_t network);
	void getIPdotAddr(std::vector<std::string>& ipv);
	void initspx(AlarmMgr* amptr);
	TOCAP::SpxState getPartnerInfo(bool& fourIPalarmIssued,
									std::vector<std::string>& ipaddr);
	void spxInfo(TOCAP::SpxState otherSpxState,
				 std::vector<std::string> ipvec_spxPartner);
	void issueAlarm(AlarmFile::fileRec& alStatus,TOCAP::TypeOfAlarm alType,std::vector<std::string>& ipvec,int net=0);
	void ceaseAlarm(AlarmFile::fileRec& alStatus,TOCAP::TypeOfAlarm alType,std::vector<std::string>& ipvec,int net=0);
    void ceaseAllAlarm(void);

	std::string nodeName;
	std::vector<Link*> links;
	TOCAP_Events* evp;
	AlarmFile* af;
	uint32_t fileIndex;
	AlarmMgr* spxPartner; // the two spx alarm objects needs to know each other.
	static std::vector<AlarmMgr*> alarmVec;

	// used by the logging function to avoid logging the same data
	bool attributesUpdated; 
	
};

#endif
