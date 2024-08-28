/*
 * client.cpp
 *
 *  Created on: oct 20, 2010
 *      Author: xtuangu
 */

#include "DSD_Client.h"
//#include "ACS_DSD_MacrosConstants.h"

#include <iostream>
#include <cstdlib>
#include <string>
#include <iostream>


using namespace std;

DSD_Client::DSD_Client()
	: connected(false),
	  client(),
	  session()
{

}

DSD_Client::~DSD_Client()
{
	// TODO Auto-generated destructor stub
}

int DSD_Client::connect()
{
	// display own node identity.
	ACS_DSD_Node my_node;

	client.get_local_node(my_node);
	cout<<"own node is:"<< my_node.node_name<< endl;;

	//int ret_result = client.connect(session,"14007", 2001, "AP1A");
	int ret_result = client.connect(session, "14007", acs_dsd::SYSTEM_ID_PARTNER_NODE, 0);

	if(ret_result < 0)
	{
		// failed to connect.
		cout<< client.last_error_text() <<endl;
		return ret_result;
	}

	connected = true;

	return ret_result;
}

int DSD_Client::disConnect()
{
	int ret_result = session.close();
	if(ret_result < 0)
	{
		cout << "Failed to close session" << endl;
	}

	connected = false;

	return ret_result;
}

bool DSD_Client::isConnected()
{
	return connected;
}


int DSD_Client::sendEcho (uint8_t value)
{
	uint8_t buf[max_length];
	buf[0] = value;

	int ret_result = session.send(buf,echo_length);
	if(ret_result < 0)
	{
			// failed to send.
		cout<< session.last_error_text() <<endl;
	}

	ret_result = session.recv(buf,max_length);
	if(ret_result)
	{
		cout<< session.last_error_text() <<endl;
	}

	cout << "Echo reply: " << (int)buf[0] << endl;
	return ret_result;
}
