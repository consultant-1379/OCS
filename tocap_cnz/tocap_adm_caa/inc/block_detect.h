//******************************************************************************
// NAME
// block_detect.h
//
// COPYRIGHT Ericsson AB, Sweden 2010.
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
// ???

// AUTHOR
// 2010-12-01 by XDT/DEK XTUANGU

// DESCRIPTION
// This class is used for block detect and release lock for OCS_OCP_frontEndAP and OCS_OCP_nonFrontEndAP.

// LATEST MODIFICATION
// -
//******************************************************************************

#ifndef block_detect_H 
#define block_detect_H

#include "OCS_OCP_protHandler.h"
#include <pthread.h>
#include "Event.h"

class Block_Detect
{
public:
	static void create_block_detect(void);
	static void delete_block_detect(void);
	static void alive(void);
	static void add_object(ProtHandler_Ptr& obj); // HY34582
	static void reset_object(void);


private:
	static void* hang_detection(void*);

	static Event stop_thread_handle;
	static ProtHandler_Ptr obj;
	static pthread_t thread_handle;

	static int aliveCnt;

};

#endif
