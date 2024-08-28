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
		SEND_PRIMITIVE_11,
		SEND_PRIMITIVE_12,
		SEND_PRIMITIVE_13,
		CONNECT_TO_AP,
		PRINT_ERROR_CODES
	};


private:
	enum MENU { MAIN = 0, MESSAGE};
//foos
public:
	Menu();
	~Menu();
	OPERATION run();
	OPERATION run_udp();
	int promptEcho(char* buf);
	int promptPrimitive11(char& sstate,uint32_t& apip,uint32_t& cpip, uint32_t& destip);
	int promptPrimitive13(uint32_t& apip1,uint32_t& apip2,uint32_t& destip);
	bool connectToAP();

private:
	Menu(const Menu& rhs);
	Menu& operator=(const Menu& rhs);
	uint8_t getEchoValue();
	void getPrimitive11(char& sstate,uint32_t& apip,uint32_t& cpip, uint32_t& destip);
	void getPrimitive13(uint32_t& apip1,uint32_t& apip2,uint32_t& destip);

//attr
private:
	MENU m_menu;
	char* m_buf;
	static const char* MENU[2];
	static const char* UDP_MENU[2];
};
//
// inlines
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif
