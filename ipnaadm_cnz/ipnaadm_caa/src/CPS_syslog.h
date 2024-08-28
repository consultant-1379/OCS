/*
COPYRIGHT Ericsson Utvecklings AB, Sweden 1999.
All rights reserved.

The Copyright to the computer program(s) herein 
is the property of Ericsson Utvecklings AB, Sweden.
The program(s) may be used and/or copied only with 
the written permission from Ericsson Utvecklings AB or in 
accordance with the terms and conditions stipulated in the 
agreement/contract under which the program(s) have been 
supplied.

NAME
	CPS_syslog.h

DESCRIPTION
	Trace/log functionality

NOTES
	Supports
		multiple output destinatons: EventLog, stdout, stderr (not buffered)
		time stamp on msgs.
		four trace levels: LVLERROR LVLWARNING LVLINFO LVLDEBUG

	Typical usage:
	1. create a global CPS_syslog object named syslog
	2. use the macros
		
		a) SYSLOG((CPS_syslog::LVLINFO, "format string %d, %s", i, buf));
		b) DBGLOG((CPS_syslog::LVLDEBUG, "format string %d, %s", i, buf));
	
	   Note the double parenthesis in a & b, required! Last semicolon is optional.

		c) DBGTRACE("<user string>"); // Uses CPS_syslog::LVLDEBUG
		   DBGTRACE(itoa(i, buf, 10));

	
	Multithreading issues:
		Externally controlled locking - use lock()/unlock as appropriate.
		The macros will do this for you.

DOCUMENT NO
	-

AUTHOR 
	000302 qabgill

*** Revision history ***
	000302 qabgill PA1 Created.

*/
/*
All files within this DLL are compiled with the CPS_SYSLOG_EXPORTS symbol defined.
This symbol should not be defined on any project that uses this DLL.
*/
#ifdef CPS_SYSLOG_EXPORTS
#define CPS_SYSLOG_API __declspec(dllexport)
#else
#define CPS_SYSLOG_API __declspec(dllimport)
#endif

#include <windows.h>
//
// scary defines ahead!
//
// SYSLOG - MUST USE DOUBLE PARENTHESIS IN CALL!
// 1. requires a syslog object
// 2. note that the "missing" parantheses round the second 'x' is deliberate
// 3. example SYSLOG((CPS_syslog::LVLWARNING, "format string %d, %s", i, buf));
//
#define SYSLOG(x) syslog.lock(); syslog.log x ; syslog.unlock();
//
// DBGLOG - MUST USE DOUBLE PARENTHESIS IN CALL!
// 1. Removed in release builds
//
#ifdef _DEBUG
#define DBGLOG(x) syslog.lock(); syslog.log x ; syslog.unlock();
#else
#define DBGLOG(x)
#endif
//
// DBGTRACE
// 1. Removed in release builds
// 2. prints out filename, linenumber, and one "user" string
// 3. don't have to use double parenthesis
#ifdef _DEBUG
#define DBGTRACE(x) \
	syslog.lock(); \
	syslog.log(CPS_syslog::LVLDEBUG, "%s > %s:%d", (x), __FILE__, __LINE__); \
	syslog.unlock()
#else
#define DBGTRACE(x)
#endif

const DWORD SYSLOG_EVENTLOG = 1;
const DWORD SYSLOG_STDOUT = 2;
const DWORD SYSLOG_STDERR = 4;

//
// This class is exported from the CPS_syslog.dll
//
class CPS_SYSLOG_API CPS_syslog {
// types
public:
	enum LVL { LVLUNDEF, LVLDEBUG, LVLINFO, LVLWARNING, LVLERROR };
	enum { MAX_MSGLEN = 512, MAX_NAMELEN = 255 };

public:
	CPS_syslog(const char* name, DWORD outputflag, LVL guardlevel, bool use_timer);
	~CPS_syslog();
	// access & modify
	LVL getGuardLevel() const { return m_guardlevel; }
	DWORD getOutputFlag() const { return m_output; }
	void setOutputFlag(DWORD dw) { m_output = dw; }
	// gen. funx
	void log(LVL level, const char* msg, ...);
	void lock() { EnterCriticalSection(&m_cs); }
	void unlock() { LeaveCriticalSection(&m_cs); }
private:
	void print(LVL level, const char* s);
	// disallowed functions
	CPS_syslog(CPS_syslog& );
	CPS_syslog& operator=(CPS_syslog& );
// members
private:
	char m_name[MAX_NAMELEN + 1];
	LVL m_guardlevel; // ex. won't print dbg msgs if this is set to warning level
	int m_output; // direct output
	bool m_timer; // use timer?
	HANDLE m_handle; // handle to nt event log
	CRITICAL_SECTION m_cs;

	static const char* LVL_STRINGS[LVLERROR + 1]; // string table
};
//
// removes the need for including/declaring syslog when the macros are used
//
extern CPS_syslog syslog;
