//******************************************************************************
// NAME
// Socket.cpp
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
// This class is use for presenting the socket.

// LATEST MODIFICATION
// -
//******************************************************************************


#include "Socket.h"
#include "OCS_OCP_Trace.h"
#include <sstream>

using namespace std;



Socket::Socket()
{
	// TODO Auto-generated constructor stub
	sockDesc = 0;

}

Socket::Socket(int type, int protocol) throw(SocketException)
{
	if ((sockDesc = ::socket(AF_INET, type, protocol)) < 0)
	{
		throw SocketException("Socket creation failed (socket())", true);
	}
}


Socket::~Socket()
{

    /*
    ::shutdown(sockDesc, SHUT_RDWR);
    ::close(sockDesc);
    sockDesc = -1;
    */
}
void Socket::closeSocket()
{
    newTRACE((LOG_LEVEL_INFO,"Socket::closeSocket()",0));

    ostringstream trace;
    trace << "Close socket #:  " << sockDesc << endl;
    TRACE((LOG_LEVEL_INFO,"%s",0 ,trace.str().c_str()));

	::shutdown(sockDesc, SHUT_RDWR);
	::close(sockDesc);
	sockDesc = -1;

}


void Socket::bind(uint32_t localAddr, uint16_t localPort) throw(SocketException)
{
	// Get the address of the requested host
	sockaddr_in localAddr_in;
	memset(&localAddr_in, 0, sizeof(localAddr_in));
	localAddr_in.sin_family = AF_INET;
	//localAddr_in.sin_addr.s_addr = htonl(localAddr);
	localAddr_in.sin_addr.s_addr = localAddr;
	localAddr_in.sin_port = htons(localPort);

	if (::bind(sockDesc, (sockaddr *) &localAddr_in, sizeof(sockaddr_in)) < 0)
	{
		throw SocketException("Set of local address and port failed (bind())", true);
	}

}

int Socket::getSocket() const
{
	return this->sockDesc;
}

void Socket::setSockOpt(int fd, int level, int optname, const void *optval, socklen_t optlen)
{
	if (::setsockopt(fd, level, optname, optval, optlen) < 0)
	{
		throw SocketException("Setsockopt failed", true);
	}
}
