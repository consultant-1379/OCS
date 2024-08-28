//******************************************************************************
// NAME
// UDPSocket.h
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

#ifndef UDPSocket_H_
#define UDPSocket_H_

#include "Socket.h"
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp> // HY34582

class UDPSocket : public Socket
{
public:
	/**
	*   Construct a TCP socket with no connection
	*   @exception SocketException thrown if unable to create TCP socket
	*/
	UDPSocket() throw(SocketException);

	/**
	*   Write the given buffer to this socket.  Call connect() before
	*   calling send()
	*   @param buffer buffer to be written
	*   @param bufferLen number of bytes from buffer to be written
	*   @param sa destination address
	*   @para salen destination socket lengh
	*   @exception SocketException thrown if unable to send data
	*/
	void sendto(const void *buffer, int bufferLen, const struct sockaddr *sa, socklen_t salen) throw(SocketException);

	/**
	*   Read into the given buffer up to bufferLen bytes data from this
	*   socket.  Call connect() before calling recv()
	*   @param buffer buffer to receive the data
	*   @param bufferLen maximum number of bytes to read into buffer
	*   @param sa destination sock addr
	*   @salenptr destination socket lengh
	*   @return number of bytes read, 0 for EOF, and -1 for error
	*   @exception SocketException thrown if unable to receive data
	*/
	int recvfrom(void *buffer, int bufferLen, struct sockaddr *sa, socklen_t *salenptr) throw(SocketException);

};

typedef boost::shared_ptr<UDPSocket> UDPSocketPtr;

#endif /* UDPSocket_H_ */
