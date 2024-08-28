//******************************************************************************
// NAME
// CommunicationSocket.cpp
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
// This class is used for socket communicating.

// LATEST MODIFICATION
// -
//******************************************************************************

#include "CommunicationSocket.h"


CommunicationSocket::CommunicationSocket(int type, int protocol) throw(SocketException)
	: Socket(type, protocol)
{

}

CommunicationSocket::CommunicationSocket(int newConnSD)
	: Socket(newConnSD)
{

}

void CommunicationSocket::connect(uint32_t foreignAddress, uint16_t foreignPort) throw(SocketException)
{
	sockaddr_in destAddr;
	memset(&destAddr, 0, sizeof(destAddr));
	destAddr.sin_family = AF_INET;
	destAddr.sin_addr.s_addr = foreignAddress;
	destAddr.sin_port = htons(foreignPort);
	// Try to connect to the given port
	if (::connect(sockDesc, (sockaddr *) &destAddr, sizeof(destAddr)) < 0)
	{
		throw SocketException("Connect failed (connect())", true);
	}
}

void CommunicationSocket::send(const void *buffer, int bufferLen) throw(SocketException)
{
	if (::send(sockDesc, (raw_type *) buffer, bufferLen, 0) < 0)
	{
		throw SocketException("Send failed (send())", true);
	}
}

int CommunicationSocket::recv(void *buffer, int bufferLen) throw(SocketException)
{
    int rtn;
	if ((rtn = ::recv(sockDesc, (raw_type *) buffer, bufferLen, 0)) < 0)
	{
		throw SocketException("Received failed (recv())", true);
	}

	return rtn;
}

void CommunicationSocket::sendto(const void *buffer, int bufferLen, const struct sockaddr *sa, socklen_t salen) throw(SocketException)
{
	if (::sendto(sockDesc, (raw_type *) buffer, bufferLen, 0, sa, salen) < 0)
	{
		throw SocketException("Send failed (sendto())", true);
	}
}

int CommunicationSocket::recvfrom(void *buffer, int bufferLen, struct sockaddr *sa, socklen_t *salenptr) throw(SocketException)
{
	int rtn;
	if ((rtn = ::recvfrom(sockDesc, (raw_type *) buffer, bufferLen, 0, sa, salenptr)) < 0)
	{
		throw SocketException("Received failed (recvfrom())", true);
	}

	return rtn;
}

string CommunicationSocket::getForeignAddress() throw(SocketException)
{
	sockaddr_in addr;
	unsigned int addr_len = sizeof(addr);

	if (getpeername(sockDesc, (sockaddr *) &addr,(socklen_t *) &addr_len) < 0)
	{
		throw SocketException("Fetch of foreign address failed (getpeername())", true);
	}

	return inet_ntoa(addr.sin_addr);
}

uint16_t CommunicationSocket::getForeignPort() throw(SocketException)
{
	sockaddr_in addr;
	unsigned int addr_len = sizeof(addr);

	if (getpeername(sockDesc, (sockaddr *) &addr, (socklen_t *) &addr_len) < 0)
	{
		throw SocketException("Fetch of foreign port failed (getpeername())", true);
	}
	return ntohs(addr.sin_port);
}
