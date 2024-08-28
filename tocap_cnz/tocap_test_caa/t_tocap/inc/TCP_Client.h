/*
 * TCP_Client.h
 *
 *  Created on: Oct 20, 2010
 *      Author: xtuangu
 */

#ifndef TCP_CLIENT_H_
#define TCP_CLIENT_H_

#include "TCP_Connection.h"
#include <string>

class TCP_Client {
public:
		explicit TCP_Client(const std::string& address, const std::string& port);
		~TCP_Client();

		// Connect to server.
		void connect();

		// Disconnect from server.
		void disconnect();

		// Check of  is connected.
		bool isConnected();

		//Send echo request to server.
		void sendEcho(uint8_t value);

		/// Run the server's io_service loop.
		void run();

		/// Stop the server.
		void stop();

		//Handle timeout to send echo to server.
		void handle_timeout(const boost::system::error_code& e);

private:
	    /// Handle a request to stop client.
	    void handle_stop();

	    /// The io_service used to perform asynchronous operations.
	    boost::asio::io_service io_service_;

	    //Timer for sending send echo request to server.
	    boost::asio::deadline_timer timer_;

	    //Connection to server.
	    TCP_Connection client_connetion;

	    std::string server_ip_addr;
	    std::string server_port;
	    bool socket_connected;

	    enum { max_length = 1024,
			   echo_interval = 8,
			   echo_length = 1//seconds
			 };
};

#endif /* TCP_CLIENT_H_ */
