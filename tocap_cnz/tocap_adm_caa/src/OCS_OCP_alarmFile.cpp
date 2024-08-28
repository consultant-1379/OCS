//******************************************************************************
// NAME
// OCS_OCP_alarmFile.cpp
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
// 2007-12-06 by EAB/FTE/DDH UABCAJN

// DESCRIPTION
// This class is a singleton class, i.e only one instance exists.
// After a process restart the alarmMgr instances are loaded with
// the alarm states in this file. Every alarmMgr instance use a
// recIndex when to set/get its data. The recIndex is mapped to
// the nodeName.

// LATEST MODIFICATION
// -
//******************************************************************************
#include "OCS_OCP_alarmFile.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <string.h>

using namespace std;



AlarmFile* AlarmFile::singletonPtr=0;
unsigned int AlarmFile::noOfUsers=0;

//******************************************************************************
// getInstance() 
//******************************************************************************
AlarmFile* AlarmFile::getInstance(int& err)
{
	err=0xABCD;
	if (singletonPtr==0)
	{
		singletonPtr=new AlarmFile();
		if (singletonPtr && (err=singletonPtr->openFile())!=0)
		{
			delete singletonPtr;
			singletonPtr = NULL;
		}
	}
	if (singletonPtr) noOfUsers++;
	return singletonPtr;
}

//******************************************************************************
// deleteInstance() 
//******************************************************************************
void AlarmFile::deleteInstance(AlarmFile* aptr)
{
	if (aptr==singletonPtr)
	{
		noOfUsers--;
		if (singletonPtr && noOfUsers==0)
		{
			delete singletonPtr;
			singletonPtr = NULL;
		}
	}
}

//******************************************************************************
// constructor
//******************************************************************************
AlarmFile::AlarmFile()
:fileName(ALARM_DIR),
 hFile(-1),
 recAddr(NO_OF_RECORDS,(fileRec*)NULL),
 mapStartAddr(NULL)
{
	fileName+=ALARM_FILE;
	singletonPtr=this;

}  


//******************************************************************************
// destructor
//******************************************************************************
AlarmFile::~AlarmFile()
{	
	if(mapStartAddr) munmap(mapStartAddr,NO_OF_RECORDS*sizeof(fileRec));
	if (hFile != -1)
	{
		close(hFile);
	}
}

//******************************************************************************
// operator()
//******************************************************************************
void AlarmFile::operator()(ofstream& of,unsigned int recIndex)
{
	if (mapStartAddr==0) return;

	of<<"Record no: "<<recIndex<<endl;
	of<<"Alarm state, Link 1: ";
	if (mapStartAddr[recIndex].linkAlarms[0]==TOCAP::CEASED) of<<"CEASED"<<endl;
	else of<<"ISSUED"<<endl;
	of<<"Alarm state, Link 2: ";
	if (mapStartAddr[recIndex].linkAlarms[1]==TOCAP::CEASED) of<<"CEASED"<<endl;
	else of<<"ISSUED"<<endl;
	of<<"Alarm state, node: ";
	if (mapStartAddr[recIndex].nodeAlarm==TOCAP::CEASED) of<<"CEASED"<<endl;
	else of<<"ISSUED"<<endl;
	of<<"Node name: "<<mapStartAddr[recIndex].nodeName<<endl;
	of<<"Number of IP-addresses: "<<mapStartAddr[recIndex].noOfIPaddresses<<endl;
}


//******************************************************************************
// openFile()
//******************************************************************************
int AlarmFile::openFile(void)
{
	int rc=0;
	long NEWSIZE = NO_OF_RECORDS*sizeof(fileRec);;

    // Open an existing or create a new registration file.
	hFile =open(fileName.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	if (hFile != -1)
	{	
		struct stat buf;
		rc = fstat(hFile, &buf );
		if(rc == -1)
			return errno;

		//if nothing in file
		if (buf.st_size == 0)
		{
			// the file can contain up to 64 alarm records.

			char clearBuf[NEWSIZE];
			memset(clearBuf,0,NEWSIZE);

			rc = write(hFile,clearBuf,NEWSIZE);
			if (rc == -1 || rc != NEWSIZE)
			{
				// cannot expand file.
				return errno;
			}
		}
		
		// Map the alarm file.
		mapStartAddr = (fileRec*)mmap((caddr_t)0, NEWSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, hFile, 0);
		if (mapStartAddr == (fileRec*)-1)
		{
			return errno;
		}

			// fill the recAddr vector with valid file-pointers.
			fileRec* fptr=mapStartAddr;
			for (int index=0;index<NO_OF_RECORDS;index++,fptr++)
			{
				if (fptr->validRecord)
				{
					recAddr[index]=fptr;
				}
			}
	}
	else
	{
		return errno;
	}

	return 0;
}


//******************************************************************************
//	set()

//  An application can either update its file data by giving a valid recIndex or
//  it can set data, and request an recIndex, by setting recIndex to UNDEFINED.

//  return values:
//  true: file updated.
//  false: error, no more available slots.	
//******************************************************************************
bool AlarmFile::set(uint32_t& recIndex,const fileRec& rec)
{
	bool retCode=false;
	if (recIndex==UNDEFINED || recIndex>=NO_OF_RECORDS)
	{
		// not yet stored in file, find an available index.
		recIndex=0;
		vector<fileRec*>::iterator i;
		for (i=recAddr.begin();i!=recAddr.end();++i,recIndex++)
		{
			if (*i==0 && mapStartAddr!=NULL)
			{
				recAddr[recIndex]=mapStartAddr+recIndex;
				memcpy(recAddr[recIndex]->nodeName, rec.nodeName, NODE_NAME_LENGTH);
				break;
			}
		}
		if (i==recAddr.end()) recIndex=UNDEFINED;
	}
	if (recIndex!=UNDEFINED && recAddr[recIndex]!=0 && strcmp(recAddr[recIndex]->nodeName,rec.nodeName)==0)
	{
		recAddr[recIndex]->validRecord=true;
		recAddr[recIndex]->linkAlarms[0]=rec.linkAlarms[0];
		recAddr[recIndex]->linkAlarms[1]=rec.linkAlarms[1];
		recAddr[recIndex]->nodeAlarm=rec.nodeAlarm;	
		recAddr[recIndex]->noOfIPaddresses=rec.noOfIPaddresses;
		msync(recAddr[recIndex],sizeof(fileRec),MS_ASYNC);
		retCode=true;
	}

	return retCode;
}

//******************************************************************************
//  get()
 
//  An application can either get its file data by giving a valid recIndex or
//  it can search (nodeName) for its file data by setting recIndex to UNDEFINED.

//  return values:
//  true: file record contains valid data.
//  false: no valid data for the node name found in file.	
//******************************************************************************
bool AlarmFile::get(uint32_t& recIndex,fileRec& rec)
{	
	bool retCode=false;
	if (recIndex==UNDEFINED  || recIndex>=NO_OF_RECORDS)
	{
		// search for the index with the correct nodename.
		recIndex=0;
		vector<fileRec*>::iterator i;
		for (i=recAddr.begin();i!=recAddr.end();++i,++recIndex)
		{
			if (*i && strcmp(recAddr[recIndex]->nodeName,rec.nodeName)==0)
			{
				break;
			}			
		}
		if (i==recAddr.end()) recIndex=UNDEFINED;
	}

	if (recIndex!=UNDEFINED && recAddr[recIndex]!=0 && strcmp(recAddr[recIndex]->nodeName,rec.nodeName)==0)
	{
		rec.validRecord=recAddr[recIndex]->validRecord;
		rec.linkAlarms[0]=recAddr[recIndex]->linkAlarms[0];
		rec.linkAlarms[1]=recAddr[recIndex]->linkAlarms[1];
		rec.nodeAlarm=recAddr[recIndex]->nodeAlarm;
		rec.noOfIPaddresses=recAddr[recIndex]->noOfIPaddresses;
		retCode=true;
	}

	return retCode;
}

