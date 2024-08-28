/*
 * OCS_OCP_frontEndAP.h
 *
 *  Created on: Dec 20, 2010
 *      Author: xtuangu
 */

#ifndef OCS_OCP_FRONTENDAP_H_
#define OCS_OCP_FRONTENDAP_H_

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

class OCS_OCP_frontEndAP
{
public:
	OCS_OCP_frontEndAP(bool multipleCP, std::string thisNodeName);
	~OCS_OCP_frontEndAP();

	TOCAP::RetCodeValue run() throw(SocketException);
	void stop() throw(SocketException);
	bool isRunning();

	static void setAPNodeStateEvent();

	static void setHWTableChangeEvent();

private:

	TOCAP::RetCodeValue createCPAPSessions();
	TOCAP::RetCodeValue createServerObjects();
	void addTestingNodes();// For testing purpose only

	std::vector<NodeData*> nodeList;
	std::vector<AlarmMgr*> aMgr_vec;
	std::vector<Link*> apLink_vec;
	std::vector<Link*> cpLink_vec;
	std::vector<HB_Session*> echo_sess;
	TOCAP_Events* evRep;
	uint32_t nodeIPaddr[2];


	ACS_DSD_Server* server_CP;

	//DSD CP server listen handles.
	acs_dsd::HANDLE cp_listen_handles[MAX_NO_OF_LISTEN_HANDLES];
	int cp_handles_count;

	//DSD Session handles
	acs_dsd::HANDLE dsd_session_handles[MAX_NO_OF_HANDLES];

	//ProtHandler* udp_ph;
	UDPSocketPtr udp_socket[NUMBER_AP_HANDLES];
	acs_dsd::HANDLE ap_listen_handles[NUMBER_AP_HANDLES];

	bool m_multipleCP;
	std::string m_thisNodeName;
	bool m_running;

	static Event s_stopEvent;
	static Event s_apNodeStateEvent;
	static Event s_hwTableChangeEvent;
};


#endif /* OCS_OCP_FRONTENDAP_H_ */
