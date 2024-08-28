/*
 * DSD_Client.h
 *
 *  Created on: Oct 20, 2010
 *      Author: xtuangu
 */

#ifndef DSD_CLIENT_H_
#define DSD_CLIENT_H_

#include <string>
#include <boost/shared_ptr.hpp>
#include "DSD_Client.h"
#include "ACS_DSD_Client.h"
#include "ACS_DSD_Session.h"

class DSD_Client {
public:
		explicit DSD_Client();
		~DSD_Client();

		int connect();
		int disConnect();
		bool isConnected();
		int sendEcho (uint8_t value);

		enum {
			max_length = 1024,
			echo_interval = 8,
			echo_length = 1
		};

private:
	    ACS_DSD_Client client;
	    ACS_DSD_Session session;

	    bool connected;

};

#endif /* DSD_CLIENT_H_ */
