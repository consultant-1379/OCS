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

#include <string.h>

using namespace std;


const char* Menu::MENU[2] = {
		"\nMAIN---\n   Q*)uit C*)onn/Disconn. M*)essages\n",
		"\nMESSAGES---\n    S*)endEcho\n"
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
int Menu::promptEcho(char* buf)
{
	uint8_t echo = getEchoValue();

	memcpy(buf, &echo, 1);
	return 1;
}

uint8_t Menu::getEchoValue()
{
	printf("Echo value (0-127): ");
	cin.getline(m_buf, 256, '\n');
	return atoi(m_buf);

}
