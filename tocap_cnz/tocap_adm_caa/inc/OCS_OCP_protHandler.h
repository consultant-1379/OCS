//******************************************************************************
// NAME
// OCS_OCP_protHandler.h
//
// COPYRIGHT Ericsson AB, Sweden 2007.
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
// 190 89-CAA 109 0748

// AUTHOR 
// 2008-07-16 by EAB/FTE/DDH UABCAJN

// DESCRIPTION
// This class handles the protocols, i.e. the CP-AP protocol and the AP-AP
// protocol. Both protocols are defined in this class.

// LATEST MODIFICATION
// -
//******************************************************************************
#ifndef _OCS_OCP_protHandler_h
#define _OCS_OCP_protHandler_h

#include "OCS_OCP_events.h"
#include "UDPSocket.h"

#include "ACS_DSD_Client.h"
#include "ACS_DSD_Session.h"


const uint8_t VER_1=1;
const uint8_t PRIM_1=1;
const uint8_t PRIM_11=11;
const uint8_t PRIM_12=12;
const uint8_t PRIM_13=13;
const uint8_t PRIM_14=14;
const uint16_t PROT_BUFSIZE=128;
const uint16_t NODENAMESIZE=16;
const uint16_t PRIM1_LENGTH=1;
const uint16_t PRIM11_LENGTH=11;
const uint16_t PRIM12_LENGTH=11;
const uint16_t PRIM13_LENGTH=10;
const uint16_t PRIM14_LENGTH=2;
const uint16_t INDEX_VER_1=1;
const uint16_t INDEX_SESS_STATE=2;
const uint16_t INDEX_APIP=3;
const uint16_t INDEX_CPIP=7;
const uint16_t INDEX_RESULT=2;
const uint16_t INDEX_APIP1=2; // primitive 13
const uint16_t INDEX_APIP2=6; // primitive 13
const uint16_t DSD_SEND_TIMEOUT = 500;
const uint16_t DSD_RECEIVE_TIMEOUT = 500;


typedef boost::shared_ptr<ACS_DSD_Session> ACS_DSD_Session_Ptr;

class ProtHandler 
{
public:
	ProtHandler(TOCAP_Events* evH);
	ProtHandler(TOCAP_Events* evH,const ACS_DSD_Session_Ptr& sessObjPtr); // HY34582
	~ProtHandler();
	static bool createUDPsock(void);
	static void deleteUDPsock(void);
	bool send_11(char sstate,uint32_t apip,uint32_t cpip,uint32_t destip);
	bool send_13(uint32_t apip1,uint32_t apip2,uint32_t destip);
	bool send_14(uint32_t destip);
	bool send_echoResp(char respVal);
	bool getPrimitive(char& prim,char& ver,bool& remoteClosed);
	bool getPrimitive_udp(char& prim,char& ver,const UDPSocketPtr& udpsocket); // HY34582
	bool get_11(char& sstate,uint32_t& apip,uint32_t& cpip);
	bool get_13(uint32_t& apip1,uint32_t& apip2);
	bool get_echo(char& echoVal);
	void closeDSD(bool kill);
    char* getSessionNodeName(void);
    char* getSessionIPAddr(void);
private:
	const ProtHandler& operator=(const ProtHandler&);
	ProtHandler(const ProtHandler&);

	bool loadBuffer(bool& remoteClosed);
	ACS_DSD_Session_Ptr sess_ptr;

	TOCAP_Events* eventRep;
	char buf[PROT_BUFSIZE];
	unsigned int bufln;
	int bufmodifier;
	int bufstored;
	static UDPSocketPtr udp_socket_ptr;

	char nodeName[NODENAMESIZE];
};

typedef boost::shared_ptr<ProtHandler> ProtHandler_Ptr;

#endif
