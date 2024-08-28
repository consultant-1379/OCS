//******************************************************************************
// NAME
// OCS_OCP_alarmFile.h
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
// 2007-11-29 by EAB/FTE/DDH UABCAJN

// DESCRIPTION
// This class is a singleton class, i.e only one instance exists.
// After a process restart the alarmMgr intances are loaded with
// the alarm states in this file. Every alarmMgr instance use a
// recIndex when to set/get its data. The recIndex is mapped to
// the nodeName.

// LATEST MODIFICATION
// -
//******************************************************************************
#ifndef OCS_OCP_alarmFile_H 
#define OCS_OCP_alarmFile_H

#include <vector>
#include <string>
#include <stdint.h>
#include "OCS_OCP_global.h"

//******************************************************************************
// class constants
//******************************************************************************
const std::string ALARM_DIR="/opt/ap/ocs/data/";
const std::string ALARM_FILE="alarmfile.bin";
const uint16_t NO_OF_RECORDS=128;
const uint32_t NODE_NAME_LENGTH=16;
const uint32_t UNDEFINED=0xFFFF;

class AlarmFile 
{
public:

	typedef struct 	     
	{
		bool    validRecord;
		TOCAP::AlarmState   linkAlarms[2]; // index 0: Eth-A, index 1 Eth-B.
		TOCAP::AlarmState   nodeAlarm;
		int    noOfIPaddresses;
		char    nodeName[NODE_NAME_LENGTH];
	} fileRec;

	// static allocation/deallocation methods.
	static AlarmFile* getInstance(int& err);
	static void deleteInstance(AlarmFile* aptr);

	// produces a readable text file with all registrations.
	void operator()(std::ofstream& of,uint32_t recIndex);

	int openFile(void);
	bool set(uint32_t& recIndex,const fileRec& rec);
	bool get(uint32_t& recIndex,fileRec& rec);

private:

	// constructors.
	AlarmFile();
	AlarmFile(const AlarmFile&);
	const AlarmFile& operator=(const AlarmFile&);

	// destructor.
	virtual ~AlarmFile();

	static AlarmFile* singletonPtr;
	static unsigned int noOfUsers;

	std::string fileName;
	int hFile;			 //Handle to the opened file.
	std::vector<fileRec*> recAddr;
	fileRec* mapStartAddr;   //Start address of the memory mapped file.
};


#endif
