//******************************************************************************
// NAME
// OCS_OCP_global.h
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
// 2008-02-14 by EAB/FTE/DDH UABCAJN

// DESCRIPTION
// contains declarations which are meant to be globally accessible.

// LATEST MODIFICATION
// -
//******************************************************************************
#ifndef _OCS_OCP_global_h
#define _OCS_OCP_global_h

#include <sys/select.h>

namespace TOCAP
{
	enum LinkState
	{
		LINK_OK_ALL_SESSIONS_UP,
      LINK_NOT_OK_SESSION_DOWN,
		LINK_NOT_OK_ALL_SESSIONS_DOWN
	};
	enum SessionState
	{
		PENDING_UP,
		PENDING_DOWN,
		UP,
		DOWN,
      UNKNOWN         /// used if NFE AP is down
	};
	enum TypeOfLink
	{
		AP,
		CPB,
		SPX
	};
	enum AlarmState
	{
		CEASED,
		ISSUED
	};
	enum SpxState
	{
		XS_NOT_DEFINED,
		EX,
		SB
	};
	enum TypeOfAlarm
	{
		NODE_ALARM,
		LINK_ALARM
	};
	enum RetCodeValue
	{
		RET_OK,
		ANOTHER_TRY,
		GETIPETHA_FAILED,
		GETIPETHB_FAILED,
		GETBOARDIDS_FAILED,
		ALLOCATION_FAILED,
		GETDEFAULTCPNAME_FAILED,
		GETCPLIST_FAILED,
		INCOMPLETE_CLUSTER,
		OPEN14007_FAILED,
		OPEN14008_FAILED,
		OPEN14009_FAILED,
		AP_GETHANDLES_FAILED,
		CP_GETHANDLES_FAILED,
		WAITFORMULTIPLE_FAILED,
		GETSYSID_FAILED,
		CONVERSION_FAILED,
		INTERNAL_RESTART,
		TIMEOUT_READ,
		AP_OPENUDPSOCKET_FAILED,
		SELECT_FAILED
	};

	enum TocapSocketPort
	{
	    TOCAP_CP = 14007,
	    TOCAP_FE = 14008,
	    TOCAP_NFE = 14009
	};

}
const int NUMBER_AP_HANDLES = 2;
const int MAX_NO_OF_HANDLES = FD_SETSIZE; // Maximize number of handles for select functions.
const int MAX_NO_OF_LISTEN_HANDLES = 8; // Maximize number of listening handles of on port 14007, 14008, 14009 for select functions.


#endif
