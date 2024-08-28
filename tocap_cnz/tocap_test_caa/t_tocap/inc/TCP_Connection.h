/*
 * TCP_Connection.h
 *
 *  Created on: Oct 20, 2010
 *      Author: xtuangu
 */

#ifndef TCP_CONNECTION_H_
#define TCP_CONNECTION_H_

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/noncopyable.hpp>


class TCP_Connection
    : private boost::noncopyable
{
public:
    /// Construct a connection with the given io_service.
    explicit TCP_Connection(boost::asio::io_service& io_service);

    /// Get the socket associated with the connection.
    boost::asio::ip::tcp::socket& socket();

    /// Get the socket associated with the connection.
    const char& getBuffer();

    /// Start the first asynchronous operation for the connection.
    void connect(const std::string& adddress, const std::string& port);

    /// Stop all asynchronous operations associated with the connection.
    void stop();

    //Send message to client
    bool sendMsg(const void* msg, std::size_t size_in_bytes);
    //Read message to buffer
    bool readMsg(void);

private:

    /// Handle completion of a read operation.
    void handle_read(const boost::system::error_code& e,
			std::size_t bytes_transferred);

    /// Handle completion of a write operation.
    void handle_write(const boost::system::error_code& e,
    		std::size_t bytes_transferred);
    /// Socket for the connection.
    boost::asio::ip::tcp::socket socket_;

    boost::asio::ip::tcp::resolver resolver_;

    /// Buffer for incoming data.
    enum { max_length = 1024 };
    uint8_t buffer_[max_length];
    unsigned int buflen;

};


#endif //OCS_TCP_CONNECTION_H
