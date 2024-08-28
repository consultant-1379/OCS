//******************************************************************************
// NAME
// Socket.h
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

#ifndef SOCKET_H_
#define SOCKET_H_


#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SocketException.h"


typedef void raw_type;       // Type used for raw data on this platform

class Socket {
public:
	Socket();
	virtual ~Socket();
	Socket(int type, int protocol) throw(SocketException);

	int getSocket() const;
	void bind(uint32_t localAddr, uint16_t localPort = 0) throw(SocketException);
	void setSockOpt(int fd, int level, int optname, const void *optval, socklen_t optlen);
	void closeSocket();

private:
	  // Prevent the user from trying to use value semantics on this object
	Socket(const Socket &sock);
	void operator=(const Socket &sock);

protected:
	int sockDesc;              // Socket descriptor

};

#endif /* SOCKET_H_ */
