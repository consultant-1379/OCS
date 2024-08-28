//******************************************************************************
// NAME
// CriticalSession.h
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
// This class is use for presenting the critical session.

// LATEST MODIFICATION
// -
//******************************************************************************

#ifndef CRITICALSECTION_H_
#define CRITICALSECTION_H_

#include <boost/thread/recursive_mutex.hpp>

class CriticalSection {

public:
	CriticalSection() : m_cs() { }
	~CriticalSection() {}
	void enter() { m_cs.lock(); }
	void leave() { m_cs.unlock(); }

private:
	boost::recursive_mutex  m_cs;
};


class AutoCS
{
public:
	AutoCS(CriticalSection& cs) : m_cs(cs) { m_cs.enter(); }
	~AutoCS() { m_cs.leave(); }

private:

	CriticalSection& m_cs;
};


#endif /* CRITICALSECTION_H_ */
