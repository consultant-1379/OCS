/*
NAME
   File_name:Menu.cpp

COPYRIGHT Ericsson Utvecklings AB, Sweden 2002. All rights reserved.

The Copyright to the computer program(s) herein is the property of Ericsson
Utvecklings AB, Sweden.
The program(s) may be used and/or copied only with the written permission from
Ericsson Utvecklings AB or in accordance with the terms and conditions
stipulated in the agreement/contract under which the program(s) have been
supplied.

DESCRIPTION
   Handles the menu I/O.

 DOCUMENT NO
   ??? ??-??? ??? ???? (190 89-CAA 109 0082)

AUTHOR
   2002-02-20 by U/Y/SF Anders Gillgren (qabgill, tag :ag)

SEE ALSO


Revision history
----------------
2002-02-20 qabgill Created

*/

#include "Menu.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <termios.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>

#include <string.h>

using namespace std;


const char* Menu::MENU[2] = {
		"\nMAIN---\n   Q*)uit C*)onn/Disconn. M*)essages\n",
		"\nMESSAGES---\n    S*)endEcho\n"
	};

const char* Menu::UDP_MENU[2] = {
		"\nMAIN---\n   Q*)uit M*)essages\n",
		"\nMESSAGES---\n    A*)sendPrimitive11. B*)sendPrimitive12. C*)sendPrimitive13. D*)connectTo./o	AP\n"
	};

int _getch( )
{
	struct termios oldt,
				 newt;
	int            ch;
	tcgetattr( STDIN_FILENO, &oldt );
	newt = oldt;
	newt.c_lflag &= ~( ICANON | ECHO );
	tcsetattr( STDIN_FILENO, TCSANOW, &newt );
	ch = getchar();
	tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
	return ch;
}

int _kbhit(void)
{
    struct timeval tv;
    fd_set read_fd;

    tv.tv_sec=0;
    tv.tv_usec=0;
    FD_ZERO(&read_fd);
    FD_SET(0,&read_fd);

    if(select(1, &read_fd, NULL, NULL, &tv) == -1)
    return 0;

    if(FD_ISSET(0,&read_fd))
    return 1;

    return 0;
}

//
// ctor
//===========================================================================
Menu::Menu() : m_menu(MAIN){
	m_buf = new char[2048];
}
//
// dtor
//===========================================================================
Menu::~Menu() {
	delete[] m_buf;
}
//
//
//===========================================================================
Menu::OPERATION Menu::run() {

	while(true)
	{
		printf(MENU[m_menu]);

		uint8_t c = toupper(_getch());

		// main
		switch(m_menu) {
			case MAIN:
				switch(c) {
				case 0x1B: // ESC
				case 'Q':
					return QUIT;
				case 'M':
					m_menu = MESSAGE;
					break;
				case 'C':
					return CONNECT;
				case 'X':
					return PRINT_ERROR_CODES;
				default:
					cout << "Unknown input" << endl;
					break;
				}
			break;
			case MESSAGE:
				switch(c) {
				case 0x1B: // ESC
				case 'Q':
					m_menu = MAIN;
					break;
				case 'S':
					return SEND_ECHO;
				case 'X':
					return PRINT_ERROR_CODES;
				default:
					cout << "Unknown input" << endl;
					break;
				}
			break;
		} // switch(m_menu)
	} // while
}

//
//
//===========================================================================
Menu::OPERATION Menu::run_udp() {

	while(true)
	{
		printf(UDP_MENU[m_menu]);

		uint8_t c = toupper(_getch());

		// main
		switch(m_menu) {
			case MAIN:
				switch(c) {
				case 0x1B: // ESC
				case 'Q':
					return QUIT;
				case 'M':
					m_menu = MESSAGE;
					break;
				case 'X':
					return PRINT_ERROR_CODES;
				default:
					cout << "Unknown input" << endl;
					break;
				}
			break;
			case MESSAGE:
				switch(c) {
				case 0x1B: // ESC
				case 'Q':
					m_menu = MAIN;
					break;
				case 'A':
					return SEND_PRIMITIVE_11;
				case 'B':
					return SEND_PRIMITIVE_12;
				case 'C':
					return SEND_PRIMITIVE_13;
				case 'D':
					return CONNECT_TO_AP;
				case 'X':
					return PRINT_ERROR_CODES;
				default:
					cout << "Unknown input" << endl;
					break;
				}
			break;
		} // switch(m_menu)
	} // while
}

//
//
//===========================================================================
int Menu::promptEcho(char* buf)
{
	uint8_t echo = getEchoValue();

	memcpy(buf, &echo, 1);
	return 1;
}

uint8_t Menu::getEchoValue()
{
	printf("Echo value (1|2|3|32): ");
	cin.getline(m_buf, 256, '\n');
	return atoi(m_buf);

}

//===========================================================================
int Menu::promptPrimitive11(char& sstate,uint32_t& apip,uint32_t& cpip, uint32_t& destip)
{
	getPrimitive11(sstate, apip, cpip, destip);
	return 1;
}

void Menu::getPrimitive11(char& sstate,uint32_t& apip,uint32_t& cpip, uint32_t& destip)
{
	printf("Session state, up/down(1/0): ");
	cin.getline(m_buf, 256, '\n');
	sstate = atoi(m_buf);

	printf("AP's IP address: ");
	cin.getline(m_buf, 256, '\n');
	apip = inet_addr(m_buf);

	printf("CP's IP address: ");
	cin.getline(m_buf, 256, '\n');
	cpip = inet_addr(m_buf);

	printf("Destination's IP address: ");
	cin.getline(m_buf, 256, '\n');
	destip = inet_addr(m_buf);
}

//===========================================================================
int Menu::promptPrimitive13(uint32_t& apip1,uint32_t& apip2,uint32_t& destip)
{
	getPrimitive13(apip1, apip2, destip);
	return 1;
}

void Menu::getPrimitive13(uint32_t& apip1,uint32_t& apip2,uint32_t& destip)
{
	printf("AP1's IP address: ");
	cin.getline(m_buf, 256, '\n');
	apip1 = inet_addr(m_buf);

	printf("Ap2's IP address: ");
	cin.getline(m_buf, 256, '\n');
	apip2 = inet_addr(m_buf);

	printf("Destination's IP address: ");
	cin.getline(m_buf, 256, '\n');
	destip = inet_addr(m_buf);
}

bool Menu::connectToAP()
{
	printf("AP's IP address: ");
	cin.getline(m_buf, 256, '\n');
	uint32_t apip = inet_addr(m_buf);
	uint16_t port = 14009;
	bool retCode = false;

	int sock, flags;
	if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) > 0)
	{
		// set socket to non-blocking mode
		flags= fcntl(sock,F_GETFL, 0 );
		fcntl(sock, F_SETFL, flags | O_NONBLOCK);

		// Get the address of the requested host
		sockaddr_in localAddr_in;
		memset(&localAddr_in, 0, sizeof(localAddr_in));
		localAddr_in.sin_family = AF_INET;
		//localAddr_in.sin_addr.s_addr = htonl(localAddr);
		localAddr_in.sin_addr.s_addr = apip;
		localAddr_in.sin_port = htons(port);

		if (connect(sock, (sockaddr *) &localAddr_in, sizeof(sockaddr_in)) < 0)
		{
			if(errno == EINPROGRESS)
			{
				fd_set readset, writeset;
				FD_ZERO(&readset);
				FD_SET(sock, &readset);
				FD_SET(sock, &writeset);
				timeval selectTimeout;
				selectTimeout.tv_sec = 0;
				selectTimeout.tv_usec = 10000;
				int select_return = select(sock+1,&readset, &writeset, NULL, &selectTimeout);
				if(select_return == 0)
				{
					cout << " select function timeout" << endl;
				}
				else if(select_return == -1)
				{
					cout << " select function failed" << endl;
				}
				else
				{
					if(FD_ISSET(sock,&readset) || FD_ISSET(sock,&writeset))
					{
						int error;
						socklen_t len;
						if(getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len) == 0)
						{
							if(error == 0)
							{
								cout << "connect successfully:" <<strerror(errno) << endl;
								retCode = true;
							}
							else
								cout << "connect failed:" <<strerror(errno) << "error:" << error << ": " << strerror(error) << endl;
						}
					}
				}
			}
			else
			{
				cout << "failed connect: " <<strerror(errno) << endl;

			}
		}
		else
		{
			cout << " connect ok:" <<strerror(errno) << endl;
			retCode = true;
		}

		fcntl(sock, F_SETFL, flags);

	}
	close(sock);

	return retCode;

}
