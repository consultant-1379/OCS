#ifndef _MENU_H
#define _MENU_H
/*
NAME
	MENU -
LIBRARY 3C++
PAGENAME MENU
HEADER  CPS
LEFT_FOOTER Ericsson Utvecklings AB
INCLUDE Menu.H

COPYRIGHT
	COPYRIGHT Ericsson Utvecklings AB, Sweden 2002. All rights reserved.

	The Copyright to the computer program(s) herein is the property of
	Ericsson Utvecklings AB, Sweden. The program(s) may be used and/or
	copied only with the written permission from Ericsson Utvecklings AB
	or in accordance with the terms and conditions stipulated in the
	agreement/contract under which the program(s) have been supplied.

DESCRIPTION
   Handles the menu I/O.

ERROR HANDLING
   -

DOCUMENT NO
	??? ??-??? ??? ???? (190 89-CAA 109 0082)

AUTHOR
   2002-02-20 by U/Y/SF Anders Gillgren (qabgill, tag: ag)

LINKAGE
	-

SEE ALSO


*/

#pragma once

#include <stdint.h>
// fwd
class TCP_Client;

class Menu {
// types
public:
	enum OPERATION {
		QUIT,
		CONNECT,
		SEND_ECHO,
		PRINT_ERROR_CODES
	};
private:
	enum MENU { MAIN = 0, MESSAGE};
//foos
public:
	Menu();
	~Menu();
	OPERATION run();
	int promptEcho(char* buf);

private:
	Menu(const Menu& rhs);
	Menu& operator=(const Menu& rhs);
	uint8_t getEchoValue();
//attr
private:
	MENU m_menu;
	char* m_buf;
	static const char* MENU[2];
};
//
// inlines
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif
