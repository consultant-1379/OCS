/*
 * OCS_OCP_nonFrontEndAP.h
 *
 *  Created on: Dec 23, 2010
 *      Author: xtuangu
 */

#ifndef OCS_OCP_NONFRONTENDAP_H_
#define OCS_OCP_NONFRONTENDAP_H_

#include "OCS_OCP_global.h"
#include "OCS_OCP_session.h"
#include "UDPSocket.h"
#include "OCS_OCP_link.h"
#include "OCS_OCP_alarmMgr.h"
#include "OCS_OCP_CSfunctions.h"
#include "ACS_DSD_Server.h"
#include "Event.h"

#include <vector>
#include <map>
#include <string>



class OCS_OCP_nonFrontEndAP
{
public:
	OCS_OCP_nonFrontEndAP(bool multipleCP,bool frontEndSystem);
	~OCS_OCP_nonFrontEndAP();

	TOCAP::RetCodeValue run() throw(SocketException);
	void stop() throw(SocketException);
	bool isRunning();
	static void setAPNodeStateEvent();

private:

	TOCAP::RetCodeValue createCPAPSessions();
	TOCAP::RetCodeValue createServerObjects();

	std::vector<NodeData*> nodeListCP;
	NodeData* myAP;
	std::vector<HB_Session*> echo_sess;
	TOCAP_Events* evRep;
	uint32_t nodeIPaddr[2];


	ACS_DSD_Server* server_CP;
	ACS_DSD_Server* server_AP;

	//DSD CP server listen handles.
	acs_dsd::HANDLE cp_listen_handles[MAX_NO_OF_LISTEN_HANDLES];
	int cp_handles_count;

	//DSD AP Server listen handles
	acs_dsd::HANDLE ap_listen_handles[MAX_NO_OF_LISTEN_HANDLES];
	int ap_handles_count;

	//DSD Session handles
	acs_dsd::HANDLE dsd_session_handles[MAX_NO_OF_HANDLES];

	//UDP AP server handles
	acs_dsd::HANDLE ap_udp_handles[NUMBER_AP_HANDLES];
	int ap_udp_handles_count;

	//ProtHandler* udp_ph;
	UDPSocketPtr udp_socket[NUMBER_AP_HANDLES];


	bool m_multipleCP;
	bool m_frontEndSystem;
	bool m_running;

	static Event s_stopEvent;
	static Event s_apNodeStateEvent;

};



#endif /* OCS_OCP_NONFRONTENDAP_H_ */
