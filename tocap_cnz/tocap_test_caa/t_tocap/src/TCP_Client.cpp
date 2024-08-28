/*
 * TCP_Client.cpp
 *
 *  Created on: oct 20, 2010
 *      Author: xtuangu
 */

#include "TCP_Client.h"

#include <cstdlib>
#include <string>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

using namespace std;

TCP_Client::TCP_Client(const std::string& address, const std::string& port)
	: io_service_(),
	  timer_(io_service_),
	  client_connetion(io_service_)


{
	// TODO Auto-generated constructor stub
	server_ip_addr = address;
	server_port = port;
	socket_connected =false;

}

TCP_Client::~TCP_Client()
{
	// TODO Auto-generated destructor stub
}


void TCP_Client::run()
{

	 //timer_.expires_from_now(boost::posix_time::seconds(1)); // initial for echo timer is 1 second.
	 //timer_.async_wait(boost::bind(&TCP_Client::handle_timeout, this,
	 //       		boost::asio::placeholders::error));

	 io_service_.run();
}

		/// Stop the server.
void TCP_Client::stop()
{
	io_service_.post(boost::bind(&TCP_Client::handle_stop, this));
}
		//Handle timeout to send echo to server.
void TCP_Client::handle_timeout(const boost::system::error_code& e)
{
	//Send echo to request to server in every 8 seconds.
	if(!e)
    {
			uint8_t request[max_length];
			//cin.getline(request, max_length);
			//size_t request_length = strlen(request);
			request[0] = 1;

			this->client_connetion.sendMsg(request, echo_length);
			this->client_connetion.readMsg();

			timer_.expires_from_now(boost::posix_time::seconds(echo_interval));
			timer_.async_wait(boost::bind(&TCP_Client::handle_timeout, this,
									boost::asio::placeholders::error));

	}
	else if (e == boost::asio::error::operation_aborted) //cancel timer.
	{
	    cout << "TCP_Client::handle_timeout exit" << endl;
	}
}

void TCP_Client::sendEcho (uint8_t value)
{
	uint8_t request[max_length];
	//cin.getline(request, max_length);
	//size_t request_length = strlen(request);
	request[0] = value;

	this->client_connetion.sendMsg(request, echo_length);
	this->client_connetion.readMsg();

}


void TCP_Client::handle_stop()
{
	 timer_.cancel();
	 client_connetion.stop();
}

void TCP_Client::connect()
{
	try
	{
		client_connetion.connect(server_ip_addr, server_port);
		socket_connected = true;
		cout <<  "Connected." << endl;
	}
	catch(std::exception& e)
	{
		cout << "Exception in connecting to server: " << e.what() << endl;
	}
}

void TCP_Client::disconnect()
{
	client_connetion.stop();
	socket_connected =false;
	cout <<  "Disconnected." << endl;
}


bool TCP_Client::isConnected()
{
	return socket_connected;
}
