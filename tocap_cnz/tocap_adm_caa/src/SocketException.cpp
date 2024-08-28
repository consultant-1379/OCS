//******************************************************************************
// NAME
// SocketException.cpp
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
// This class is use for presenting the socket exception.

// LATEST MODIFICATION
// -
//******************************************************************************

#include "SocketException.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>



SocketException::SocketException(const string &message, bool inclSysMsg)
  throw() : userMessage(message)
{
	 if (inclSysMsg)
	 {
		 userMessage.append(": ");
		 userMessage.append(strerror(errno));
	 }
}

SocketException::~SocketException() throw()
{

}

const char *SocketException::what() const throw()
{
	return userMessage.c_str();
}
