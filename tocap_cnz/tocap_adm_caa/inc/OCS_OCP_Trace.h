#ifndef _OCS_OCP_Trace_H_
#define _OCS_OCP_Trace_H_
/*
NAME
	File_name:OCS_OCP_Trace.cpp

Ericsson Utvecklings AB

	COPYRIGHT Ericsson Utvecklings AB, Sweden 2014. All rights reserved.

	The Copyright to the computer program(s) herein is the property of
	Ericsson Utvecklings AB, Sweden. The program(s) may be used and/or
	copied only with the written permission from Ericsson Utvecklings AB
	or in accordance with the terms and conditions stipulated in the 
	agreement/contract under which the program(s) have been supplied.

DESCRIPTION
	Class implements a debug context class used in the development of the 
	reset of the code. Note that no methods are to be called directly, 
	rather by use of macros so that the debug code may be conditionally 
	compiled out.

DOCUMENT NO


AUTHOR
	2014-02-25 by DEK/xtuangu

SEE ALSO
	-

Revision history
----------------
2014-02-25 xtuangu Created


*/

#include <ACS_TRA_trace.h>
#include <ACS_TRA_Logging.h>

#include "boost/scoped_ptr.hpp"
#include "CriticalSection.h"

class OCS_OCP_Trace
{
public:

	OCS_OCP_Trace();
	~OCS_OCP_Trace();
	void newTrace(ACS_TRA_LogLevel log_level, const char *fmt, int dummy, ...);
	void Trace(ACS_TRA_LogLevel log_level, const char *fmt, int dummy, ...);
	
	static bool isTraceable();
	static bool isLogable();
	static bool isTraceLogable();
	static void Initialise();
	static void Terminate();
	 
private:

	OCS_OCP_Trace(const OCS_OCP_Trace&);
	OCS_OCP_Trace& operator=(const OCS_OCP_Trace&);

	static int         s_nestTraceLevel;     // call nesting level
	static int         s_nestLogLevel; 
	static int         s_maxNestLevel;       // maximum call nesting level for trace output
	static CriticalSection s_cs;
	
	int				  m_nestTraceLevel;
	int				  m_nestLogLevel;
	bool              m_newTrace;
	bool              m_newLog;
	
	static boost::scoped_ptr<ACS_TRA_trace> s_trcb; 
    static boost::scoped_ptr<ACS_TRA_Logging> s_logcb;

};


inline
OCS_OCP_Trace::OCS_OCP_Trace() : m_nestTraceLevel(0), m_nestLogLevel(0), m_newTrace(true), m_newLog(true)
{
}

inline
bool OCS_OCP_Trace::isTraceable()
{
    return s_nestTraceLevel <= s_maxNestLevel && s_trcb->isOn();
}

inline
bool OCS_OCP_Trace::isLogable()
{
    return s_nestLogLevel <= s_maxNestLevel;
}

inline
bool OCS_OCP_Trace::isTraceLogable()
{
    return (s_nestTraceLevel <= s_maxNestLevel && s_trcb->isOn()) || (s_nestLogLevel <= s_maxNestLevel);
}

#define initTRACE()								\
        OCS_OCP_Trace::Initialise();

#define isTRACEABLE()                           \
        OCS_OCP_Trace::isTraceLogable()
        
#define newTRACE(p1)							\
        OCS_OCP_Trace Trace;                  \
        if(OCS_OCP_Trace::isTraceLogable()) { \
            Trace.newTrace p1;                  \
        }
 
#define TRACE(p1)								\
        if(OCS_OCP_Trace::isTraceLogable()) { \
            Trace.Trace p1;                     \
        }                 

#define termTRACE()								\
        OCS_OCP_Trace::Terminate();

#endif

