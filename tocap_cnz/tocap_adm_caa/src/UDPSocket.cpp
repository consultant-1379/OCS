//******************************************************************************
// NAME
// UDPSocket.cpp
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
// This class is use for presenting the UDP Socket.

// LATEST MODIFICATION
// -
//******************************************************************************

#include "UDPSocket.h"

UDPSocket::UDPSocket() throw(SocketException)
	: Socket(SOCK_DGRAM, IPPROTO_UDP)
{

}

void UDPSocket::sendto(const void *buffer, int bufferLen, const struct sockaddr *sa, socklen_t salen) throw(SocketException)
{
	if (::sendto(sockDesc, (raw_type *) buffer, bufferLen, 0, sa, salen) < 0)
	{
		throw SocketException("Send failed (sendto())", true);
	}
}

int UDPSocket::recvfrom(void *buffer, int bufferLen, struct sockaddr *sa, socklen_t *salenptr) throw(SocketException)
{
	int rtn;
	if ((rtn = ::recvfrom(sockDesc, (raw_type *) buffer, bufferLen, 0, sa, salenptr)) < 0)
	{
		throw SocketException("Received failed (recvfrom())", true);
	}

	return rtn;
}
