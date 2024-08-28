/*
NAME
	File_name:OCS_OCP_Trace.cpp

Ericsson Utvecklings AB

	COPYRIGHT Ericsson Utvecklings AB, Sweden 2000. All rights reserved.

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
2002-10-10 uabmnst Created
2010-12-31 xdtthng Remove global object; 
           Follow DR, only collect trace data if traceable;
           Keep exact functionality as Windows version

*/


#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <iostream>

#include "OCS_OCP_Trace.h"

int	OCS_OCP_Trace::s_nestTraceLevel = 0;
int	OCS_OCP_Trace::s_nestLogLevel = 0;
int	OCS_OCP_Trace::s_maxNestLevel = 14;

CriticalSection OCS_OCP_Trace::s_cs;

boost::scoped_ptr<ACS_TRA_trace> OCS_OCP_Trace::s_trcb;       // Trace object, used to be known as trace control block
boost::scoped_ptr<ACS_TRA_Logging> OCS_OCP_Trace::s_logcb;    // Log object

/*
// Log level for trace log
enum ACS_TRA_LogLevel
{
	LOG_LEVEL_TRACE,
	LOG_LEVEL_DEBUG,
	LOG_LEVEL_INFO,
	LOG_LEVEL_WARN,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_FATAL
};
*/


OCS_OCP_Trace::~OCS_OCP_Trace()
{
	AutoCS a(s_cs);
	if (!m_newTrace)
	   --s_nestTraceLevel;
	if (!m_newLog)
	   --s_nestLogLevel;
}


//******************************************************************************
//	newTrace
//******************************************************************************
void OCS_OCP_Trace::newTrace(ACS_TRA_LogLevel logLevel, const char *fmt, int dummy, ...)
{
	char       l_outText[512];
	char       l_rawText[256];
	va_list    l_args;
	
	bool traceable = isTraceable();
	bool logable = isLogable();
	//ACS_TRA_LogLevel logLevel = (dummy < LOG_LEVEL_TRACE || dummy > LOG_LEVEL_FATAL)?
	//		LOG_LEVEL_TRACE : static_cast<ACS_TRA_LogLevel>(dummy);

	//std::cout << "dummy is: " << dummy << " logLevel is " << logLevel << std::endl;

    // At this point, either or both Trace or Log is On
    // Need to check one by one	   	
    {
	    AutoCS a(s_cs);
	    if (traceable)
            m_nestTraceLevel = m_newTrace? m_newTrace = false, s_nestTraceLevel++ : s_nestTraceLevel;
        if (logable)
            m_nestLogLevel = m_newLog? m_newLog = false, s_nestLogLevel++ : s_nestLogLevel;
    }
	
    // Collect arguments once for either or both trace and log
	va_start(l_args, dummy);
	vsnprintf(l_rawText, 255, fmt, l_args);
	l_rawText[255] = '\0';
	va_end(l_args);

    // Only Trace if Trace is On
    int l_index;    
	if (traceable) {	
    	l_index = sprintf(l_outText, "<%d> %*s", m_nestTraceLevel, (m_nestTraceLevel + 1)*2, " ");
    	sprintf(&l_outText[l_index], "%s", l_rawText);
		s_trcb->ACS_TRA_event(1, l_outText);
	}

	// Only Log if log level has not exceed max level
	if (logable) {	
        l_index = sprintf(l_outText, "<%d> %*s", m_nestLogLevel, (m_nestLogLevel + 1)*2, " ");
        sprintf(&l_outText[l_index], "%s", l_rawText);
        s_logcb->Write(l_outText, logLevel);
    }	
   
}

//******************************************************************************
//	Trace()
//******************************************************************************
void OCS_OCP_Trace::Trace(ACS_TRA_LogLevel logLevel, const char *fmt, int dummy, ...)
{
	char       l_outText[512];
	char       l_outText_log[512]; //trautil text
	char       l_lineout[1024];
	char*      l_pLine;
	char*      last;
	   	
    va_list l_args;
	va_start(l_args, dummy);
	vsnprintf(l_outText, 511, fmt, l_args);
	l_outText[511] = '\0';
	va_end(l_args);

	memcpy(l_outText_log, l_outText, 511);

    // Only Trace if Trace is On
	if (isTraceable()) {	
    	l_pLine = strtok_r(l_outText, "\n", &last);
    	while (l_pLine) {
    		sprintf(l_lineout, "<%d> %*s %s", m_nestTraceLevel, m_nestTraceLevel*2+1, " ", l_pLine);          
            s_trcb->ACS_TRA_event(1, l_lineout);
    	    l_pLine = strtok_r(NULL, "\n", &last);
        }
    }

	// Only Log if log level has not exceeded max level
	if (isLogable()) {	
	   l_pLine = strtok_r(l_outText_log, "\n", &last);
    	while (l_pLine) {
        	sprintf(l_lineout, "<%d> %*s %s", m_nestLogLevel, m_nestLogLevel*2+1, " ", l_pLine);          
            s_logcb->Write(l_lineout, logLevel);
            l_pLine = strtok_r(NULL, "\n", &last);
        }
    }
}

//******************************************************************************
//	Terminate()
//******************************************************************************
void OCS_OCP_Trace::Terminate()
{
    s_trcb.reset();
    s_logcb.reset();
}

//******************************************************************************
//	Initialise()
//******************************************************************************
void OCS_OCP_Trace::Initialise()
{
	
	s_trcb.reset(new ACS_TRA_trace("", "C380"));
	s_logcb.reset(new ACS_TRA_Logging());
	s_logcb->Open("TOCAP");

	char *l_envptr = getenv("CPS_OCP_MAXNESTLEVEL");
	if (l_envptr)
	{
		int level = atoi(l_envptr);
		s_maxNestLevel = ((level > 0) ? level : s_maxNestLevel);
	}
}
