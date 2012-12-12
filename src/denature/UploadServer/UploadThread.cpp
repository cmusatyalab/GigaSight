/*
 * UploadThread.cpp
 *
 *  Created on: Oct 11, 2012
 *      Author: Pieter Simoens
 */

#include "UploadThread.h"
#include "UploadManager.h"

#include <iostream>
#include <boost/thread.hpp>
#include <iostream>
#include <sys/socket.h>
#include <errno.h>
#include <cstring>
#include "UploadProtocol.h"
#include "SocketUtils.h"
#include <fstream>


namespace upload {

UploadThread::UploadThread() {
	fd = 0;
	keepRunning = true;
}

UploadThread::~UploadThread() {

}

//see comment in UploadManager.cpp on why this method is needed and we cannot call directly a method in a derived class
//has to do with housekeeping of created objects in UploadManager
void UploadThread::start(int fd, SharedQueue<SegmentInformation*>* s){
	this->fd = fd;
	keepRunning = true;
	s_queue = s;
	startReceive();
	cleanUp();
}

void UploadThread::cleanUp(){
	close(fd);
}

void UploadThread::startReceive(){

	std::cout << "Start receiving file on FD" << fd << std::endl;

	//read in resourceURI length
	int URIlength;

	int ret = recvExact(fd, (char *) &URIlength, sizeof(int));
	if (ret <= 0) {
			if (ret == 0)
				std::cerr << "Client closed connection, could not read in size of URI"
								<< std::endl;
			else {

				std::cerr << "Could not read in size of URI: " << strerror(errno)
							<< std::endl;

				UploadProtocol::reply(fd, UploadProtocol::STATUS_ERROR);
			}

			close(fd);

			return;
	}

	URIlength = ntohl(URIlength);
	//std::cout << "URIlength: " << URIlength << std::endl;

	//read in resourceURI and reply to client
	char URIarray[URIlength];

	ret = recvExact(fd, URIarray, URIlength);
	if (ret <= 0) {
			if (ret == 0)
				std::cerr << "Client closed connection, could not read in URI"
								<< std::endl;
			else {
				std::cerr << "Could not read in URI: " << strerror(errno)
								<< std::endl;
				UploadProtocol::reply(fd, UploadProtocol::STATUS_ERROR);
			}
			close(fd);
			return;
	}


	//take care, the char array is not null-terminated!
	std::string URI = std::string(URIarray, URIlength);
	std::cout << "URI: " << URI << std::endl;


	UploadProtocol::reply(fd, UploadProtocol::STATUS_OK);


	//read in the size of the file
	unsigned int fileSize;

	ret = recvExact(fd, (char *) &fileSize, sizeof fileSize);

	if (ret <= 0) {
			if (ret == 0)
				std::cerr << "Client closed connection, could not read in fileSize" << std::endl;
			else {
				std::cerr << "Could not read in fileSize: " << strerror(errno) << std::endl;
				UploadProtocol::reply(fd, UploadProtocol::STATUS_ERROR);
			}

			return;

	}


	fileSize = ntohl(fileSize);
	//std::cout << "Will receive file of " << fileSize << " bytes" << std::endl;

	//send OK
	//std::cout << "Sending OK" << std::endl;
	UploadProtocol::reply(fd, UploadProtocol::STATUS_OK);

	std::ofstream outputFile;
	// URI is /segment/segmentID/streamID/
	std::string fileName = URI.substr((URI.find_first_of("/", 9)+1), (URI.find_last_of("/", URI.size())-URI.find_first_of("/", 9)-1));

	std::cout << "write to file path " << fileName << std::endl;

	outputFile.open(fileName.c_str());

	if (!outputFile.is_open()){
			UploadProtocol::reply(fd, UploadProtocol::STATUS_ERROR);
			std::cout << "cannot write to " << fileName << std::endl;
			return;
	}

	unsigned int offset = 0;
	char data[10240];

	while ((offset < fileSize) && keepRunning) {

		//std::cout << "remaining bytes: " << (fileSize-offset) << std::endl;

		ret = recv(fd, ((char *) &data), 10240, 0);

		if (ret < 0) {

			std::cerr << "could not read in bytes from socket: "
					<< strerror(errno) << std::endl;
			UploadProtocol::reply(fd, UploadProtocol::STATUS_ERROR);
			return;

		}

		outputFile.write(data, ret);

		bzero(data, 10240);

		offset += ret;

	}

	//send OK
	UploadProtocol::reply(fd, UploadProtocol::STATUS_OK);

	std::cout << "Received: " << offset << " bytes" << std::endl;
	finishing(URI);
}
} /* namespace upload */
