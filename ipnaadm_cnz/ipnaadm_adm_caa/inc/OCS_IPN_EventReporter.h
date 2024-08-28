//******************************************************************************
// NAME
// OCS_IPN_EventReporter.h
//
// COPYRIGHT Ericsson AB, Sweden 2012.
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
// 190 89-CAA 109 1405

// AUTHOR 
// 2012-02-23 by XDT/DEK XTUANGU

// DESCRIPTION
// This class handles events to be reported to AEH.

// LATEST MODIFICATION
// -
//******************************************************************************
#ifndef OCS_IPN_EventReporter_H
#define OCS_IPN_EventReporter_H

#include <string>
#include <vector>
#include "acs_aeh_evreport.h"

typedef struct 
{
	std::string probableCause; 
    std::string objectOfRef;
    std::string problemData;
    std::string problemText;    
} EventRec;

const EventRec INITEVENT={"","","",""};

class OCS_IPN_EventReporter
{
public:
	static OCS_IPN_EventReporter* getInstance(void);
	static void deleteInstance(OCS_IPN_EventReporter* tptr);

	void reportEvent(int problemNo,std::string errstr,std::string objRef="");

private:
	OCS_IPN_EventReporter();
	~OCS_IPN_EventReporter();

	static OCS_IPN_EventReporter* singletonPtr;
	static unsigned int noOfUsers;

	std::string procName;
	std::vector<EventRec> events;
	acs_aeh_evreport* evp;
};

#endif
