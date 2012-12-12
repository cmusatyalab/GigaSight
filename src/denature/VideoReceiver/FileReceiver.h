/*
 * FileReceiver.h
 *
 *  Created on: Oct 23, 2012
 *      Author: yuxiao
 */

#ifndef FILERECEIVER_H_
#define FILERECEIVER_H_

#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::udp;

class FileReceiver {
public:
	FileReceiver(boost::asio::io_service& io_service, short port);
	virtual ~FileReceiver();

	void handle_receive_from(const boost::system::error_code& error,size_t bytes_recvd);

	void handle_send_to(const boost::system::error_code& error, size_t bytes_sent);

	void operator()();

private:
	unsigned short port_;
	int protocol;//0: UDP
	unsigned int fileSize;
	unsigned int receivedSize;

	boost::asio::io_service& io_service_;
	udp::socket socket_;
	udp::endpoint sender_endpoint_;
	enum { max_length = 1024 };
	char data_[max_length];
};

#endif /* FILERECEIVER_H_ */
