//******************************************************************************
// NAME
// CommunicationSocket.h
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

#ifndef COMMUNICATIONSOCKET_H_
#define COMMUNICATIONSOCKET_H_

#include "SocketException.h"
#include "Socket.h"


class CommunicationSocket : public Socket
{
public:
	/**
	*   Establish a socket connection with the given foreign
	*   address and port
	*   @param foreignAddress foreign address
	*   @param foreignPort foreign port
	*   @exception SocketException thrown if unable to establish connection
	*/
	void connect(uint32_t foreignAddress, uint16_t foreignPort) throw(SocketException);

	/**
	*   Write the given buffer to this socket.  Call connect() before
	*   calling send()
	*   @param buffer buffer to be written
	*   @param bufferLen number of bytes from buffer to be written
	*   @exception SocketException thrown if unable to send data
	*/
	void send(const void *buffer, int bufferLen) throw(SocketException);

	/**
	*   Read into the given buffer up to bufferLen bytes data from this
	*   socket.  Call connect() before calling recv()
	*   @param buffer buffer to receive the data
	*   @param bufferLen maximum number of bytes to read into buffer
	*   @return number of bytes read, 0 for EOF, and -1 for error
	*   @exception SocketException thrown if unable to receive data
	*/
	int recv(void *buffer, int bufferLen) throw(SocketException);


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

	/**
	*   Get the foreign address.  Call connect() before calling recv()
	*   @return foreign address
	*   @exception SocketException thrown if unable to fetch foreign address
	*/
	string getForeignAddress() throw(SocketException);

	/**
	*   Get the foreign port.  Call connect() before calling recv()
	*   @return foreign port
	*   @exception SocketException thrown if unable to fetch foreign port
	*/
	uint16_t getForeignPort() throw(SocketException);

	protected:
	CommunicationSocket(int type, int protocol) throw(SocketException);
	CommunicationSocket(int newConnSD);
};

#endif /* COMMUNICATIONSOCKET_H_ */
