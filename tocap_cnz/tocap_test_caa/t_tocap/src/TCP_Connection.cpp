/*
 * TCP_Connection.cpp
 *
 *  Created on: Oct 20, 2010
 *      Author: xtuangu
 */

#include "TCP_Connection.h"

#include <vector>
#include <boost/bind.hpp>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <boost/asio/detail/socket_types.hpp>


using namespace std;
using boost::asio::ip::tcp;

TCP_Connection::TCP_Connection(boost::asio::io_service& io_service)
  : socket_(io_service),
    resolver_(io_service)
{

}

boost::asio::ip::tcp::socket& TCP_Connection::socket()
{
  return socket_;
}

void TCP_Connection::connect(const std::string& adddress, const std::string& port)
{

	tcp::resolver::query query(tcp::v4(), adddress, port);
	tcp::resolver::iterator iterator = resolver_.resolve(query);
	socket_.connect(*iterator);

}

void TCP_Connection::stop()
{
    try
    {
    	cout << "close socket" << endl;
    	//socket_.shutdown(tcp::socket::shutdown_both);
    	socket_.close();
    }
    catch (std::exception& e)
    {
    	cout << "stop exception: "  << e.what() << endl;
    }
}

void TCP_Connection::handle_read(const boost::system::error_code& e,
    std::size_t bytes_transferred)
{
	cout << "handle_read" << endl;

	if (!e)
    {
        //Print Server IP address
		boost::system::error_code ec;
		boost::asio::ip::tcp::endpoint endpoint = socket_.remote_endpoint(ec);
		if (!ec)
		{
			cout << "Server's IP address: %s" << endpoint.address().to_string().c_str() << endl;

		}

		cout << "Echo reply" << (int)buffer_[0] << endl;
    }
    else if (e != boost::asio::error::operation_aborted)
    {

    }

}

void TCP_Connection::handle_write(const boost::system::error_code& e,
									std::size_t bytes_transferred)
{

}

bool TCP_Connection::sendMsg(const void* msg, std::size_t size_in_bytes)
{
	cout << "Echo request: " << (int)((char*)msg)[0] << endl;

	try
	{
		boost::asio::write(socket_, boost::asio::buffer(msg, size_in_bytes));
		/*
		boost::asio::async_write(socket_,
					  boost::asio::buffer(msg, size_in_bytes),
					  boost::bind(&TCP_Connection::handle_write, this,
							  boost::asio::placeholders::error,
							  boost::asio::placeholders::bytes_transferred));
		*/
	}
	catch (std::exception& e)
	{
		cout << "sendMsg exception: "  << e.what() << endl;
	}
	return 0;
}

bool TCP_Connection::readMsg(void)
{
		//Reset buffer_
	memset(this->buffer_, 0 ,max_length);

	/*
	socket_.async_read_some(boost::asio::buffer(buffer_, max_length),
			        boost::bind(&TCP_Connection::handle_read, this,
			            boost::asio::placeholders::error,
			            boost::asio::placeholders::bytes_transferred));
	*/

	try
	{
		socket_.read_some(boost::asio::buffer(buffer_, max_length));

		//Print Server IP address
		boost::system::error_code ec;
		boost::asio::ip::tcp::endpoint endpoint = socket_.remote_endpoint(ec);
		if (!ec)
		{
		cout << "Server's IP address: " << endpoint.address().to_string().c_str() << endl;

		}

		cout << "Echo reply: " << (int)buffer_[0] << endl;
	}
	catch(std::exception& e)
	{
		cout << "readMsg exception: " << e.what() << endl;
	}
	return 0;
}


