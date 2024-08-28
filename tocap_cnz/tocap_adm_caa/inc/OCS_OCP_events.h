//******************************************************************************
// NAME
// OCS_OCP_events.h
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
// 2007-10-22 by EAB/FTE/DDH UABCAJN

// DESCRIPTION
// This class handles events and alarms to be reported to AEH.

// LATEST MODIFICATION
// -
//******************************************************************************
#ifndef OCS_OCP_events_h
#define OCS_OCP_events_h

#include <string>
#include <vector>
#include "OCS_OCP_global.h"
#include "acs_aeh_evreport.h"

typedef struct 
{
	std::string probableCause; 
    std::string objectOfRef;
    std::string problemData;
    std::string problemText;    
} EventRec;

const EventRec INITEVENT={"","","",""};

class TOCAP_Events
{
public:
	static TOCAP_Events* getInstance(void);
	static void deleteInstance(TOCAP_Events* tptr);

	void reportEvent(int problemNo,std::string errstr,std::string objRef="");
	void alarmUpdate(std::string alarmaction,TOCAP::TypeOfAlarm toa,std::string nodeName,std::vector<std::string> ip, int net=0);

private:
	TOCAP_Events();
	~TOCAP_Events();
    bool tooFrequent(int problemNo);

	static TOCAP_Events* singletonPtr;
	static unsigned int noOfUsers;

	std::string procName;
	std::vector<EventRec> events;
	std::vector<int> eventCnt;
	std::vector<time_t> eventTime;
	acs_aeh_evreport* evp;
};

#endif
