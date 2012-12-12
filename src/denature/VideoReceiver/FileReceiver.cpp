  //
// async_udp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "FileReceiver.h"
#include <iostream>
#include <fstream>
#include <cstring>

FileReceiver::FileReceiver(boost::asio::io_service& io_service, short port)
     : io_service_(io_service),
       socket_(io_service, udp::endpoint(udp::v4(), port))
{
	fileSize = 0;
	receivedSize = 0;
}




FileReceiver::~FileReceiver() {
	// TODO Auto-generated destructor stub
}

void FileReceiver::operator()(){


	socket_.async_receive_from(
       boost::asio::buffer(data_, 4), sender_endpoint_,
       boost::bind(&FileReceiver::handle_receive_from, this,
       boost::asio::placeholders::error,
       boost::asio::placeholders::bytes_transferred));
}



void FileReceiver::handle_receive_from(const boost::system::error_code& error, size_t bytes_recvd)
{
	std::ofstream outputFile;
	outputFile.open("recorded1.mp4");

    if (!error && bytes_recvd > 0)
    {
    	if (fileSize == 0)
    	{
    		char temp[sizeof(fileSize)] = { 0 };
    		strncpy(temp, data_, sizeof(fileSize));
    		fileSize = atoi(temp);

    		std::cout << "file Size" << fileSize << std::endl;

    	}
    	else{

    		receivedSize += bytes_recvd;

			//write to local file
			outputFile << data_;


			if (receivedSize < fileSize){

				socket_.async_receive_from(
					   boost::asio::buffer(data_, max_length), sender_endpoint_,
					   boost::bind(&FileReceiver::handle_receive_from, this,
					   boost::asio::placeholders::error,
					   boost::asio::placeholders::bytes_transferred));
			}

        }
    }
    else
        outputFile.close();

}
