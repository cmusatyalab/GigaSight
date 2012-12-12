/*
 * UploadManager.cpp
 *
 *  Created on: Oct 11, 2012
 *      Author: Pieter Simoens
 */


#include <iostream>
#include "UploadManager.h"
#include "GPSUpload.h"
#include "MP4FileUpload.h"
#include "RTSP_RTPUpload.h"
#include "SocketUtils.h"

namespace upload {

//SINGLETON stuff
UploadManager* UploadManager::MInstance = 0;

UploadManager& UploadManager::Instance(SharedQueue<SegmentInformation*>* s) {
	if (MInstance == 0)
		MInstance = new UploadManager(s);
	return *MInstance;
}

UploadManager::UploadManager(SharedQueue<SegmentInformation*>* s) {
	this->keepRunning = true;
	listenPort = UploadManager::DEFAULTPORT;
	listenFD = 0;
	s_queue = s;
	atexit(&CleanUp);
}

void UploadManager::CleanUp() {
	delete MInstance;
	MInstance = 0;
} // Note the = 0 bit!!!

//helper function get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {

	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*) sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

//note: it can take a while before the listen socket is actually on... So if you immediately start an upload
//after starting this thread, you might be rejected..
void UploadManager::run() {

	int new_fd;
	fd_set fd;
	timeval tv; //timeout for select, so that the keepRunning flag is regularly checked and thread can be stopped
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	struct sockaddr_storage client_addr; // client address information
	socklen_t sin_size;

	int port = listenPort;

	while((listenFD = openListenSocket(port)) < 0){
		port++;

		if ((port-listenPort) > 5)
			return;
		//return; //stop the UploadManager
	}

	std::cout << "UploadManager listening on port " << port << std::endl;

	while (keepRunning) {  // main accept() loop
		FD_ZERO(&fd);
		FD_SET(listenFD, &fd);
		if (select(listenFD + 1, &fd, NULL, NULL, &tv) > 0) {
			sin_size = sizeof client_addr;
			std::cout << "New connection detected" << std::endl;
			new_fd = accept(listenFD, (struct sockaddr *) &client_addr, &sin_size);
			if (new_fd == -1) {
				std::cerr << "accept" << strerror(errno) << std::endl;
				continue;
			}

			char s[INET6_ADDRSTRLEN];
			inet_ntop(client_addr.ss_family,
					get_in_addr((struct sockaddr *) &client_addr), s, sizeof s);

			std::cout << "Got connection from " << s << " on FD" << new_fd
					<< std::endl;

			handleNewConnection(new_fd);
		} //time-out of select, evaluate if thread should till be running
	}

	//thread was asked to stop, do cleanup here
	//todo: what with still running uploads?

	close(listenFD);
	std::cout << "UploadManager stopped" << std::endl;
}

void UploadManager::handleNewConnection(int recvFD) {

	//read in resourceURI length
	//move the following code to MP4FileUpload.cpp
	/*	int URIlength;

		int ret = recvExact(recvFD, (char *) &URIlength, sizeof(int));
		if (ret <= 0) {
			if (ret == 0)
				std::cerr
						<< "Client closed connection, could not read in size of URI"
						<< std::endl;
			else {
				std::cerr << "Could not read in size of URI: " << strerror(errno)
						<< std::endl;
				UploadProtocol::reply(recvFD, UploadProtocol::STATUS_ERROR);
			}
			close(recvFD);
			return;
		}

		URIlength = ntohl(URIlength);
		std::cout << "URIlength: " << URIlength << std::endl;

		//read in resourceURI and reply to client
		char URIarray[URIlength];

		ret = recvExact(recvFD, URIarray, URIlength);
		if (ret <= 0) {
			if (ret == 0)
				std::cerr << "Client closed connection, could not read in URI"
						<< std::endl;
			else {
				std::cerr << "Could not read in URI: " << strerror(errno)
						<< std::endl;
				UploadProtocol::reply(recvFD, UploadProtocol::STATUS_ERROR);
			}
			close(recvFD);
			return;
		}


		//take care, the char array is not null-terminated!
		std::string URI = std::string(URIarray, URIlength);
		std::cout << "URI: " << URI << std::endl;


		UploadProtocol::reply(recvFD, UploadProtocol::STATUS_OK);*/

	MP4FileUpload upThread;
	boost::thread b_thread(boost::bind(&upload::MP4FileUpload::start, upThread,
									recvFD, s_queue));



	//start the correct UploadThread type
	//note: we only close recvFD on errors, as it is still needed in the started thread!
	/*prm::pResource_t pRes;
	if (prm::PrivateResourceManager::Instance().read(URI, pRes) == prm::ok) {
		//send OK to server
		UploadProtocol::reply(recvFD,UploadProtocol::STATUS_OK);

		if (prm::pStreamResource_t derived = boost::dynamic_pointer_cast<
				prm::StreamResource>(pRes)) {
			switch (derived->container) {
			case prm::StreamResource::CONTAINER_MP4: {
				MP4FileUpload upThread;
				//we had to implement a non-virtual start()-method in UploadThread, that subsequently calls the virtual
				//startReceive functions in the derived classes. If we would call directly the startReceive function,
				//which would in that case be purely virtual in UploadThread, we encounter runtime errors.
				//In that case, the object upThread is deleted before the boost::bind call can call the actual method (multithreading issues...)
				//an alternative can be to create upThread with the new method, but then you have
				//to do some housekeeping: e.g. MP4Upload thread must then say it is finished, so the object
				//can be destroyed here...
				boost::thread b_thread(
						boost::bind(&upload::MP4FileUpload::start, upThread,
								recvFD));
				return;
			}
			case prm::StreamResource::CONTAINER_GPS: {
				GPSUpload upThread;
				//boost::thread b_thread(boost::bind(&upload::GPSUpload::startReceive, &upThread));
				return;
			}
			case prm::StreamResource::CONTAINER_RTSP_RTP: {
				RTSP_RTPUpload upThread;
				//boost::thread b_thread(boost::bind(&upload::RTSPRTPUpload::startReceive, &upThread));
				return;
			}
			default:
				std::cerr
						<< "Unknown container format of stream, closing connection"
						<< std::endl;
				UploadProtocol::reply(recvFD, UploadProtocol::STATUS_ERROR);
				close(recvFD);
			}
		} else {
			std::cerr
					<< "ResourceID is not a stream, closing connection to client"
					<< std::endl;
			UploadProtocol::reply(recvFD, UploadProtocol::STATUS_ERROR);
			close(recvFD);
			return;
		}

	} else {
		std::cerr << "Wrong resource ID, closing connection" << std::endl;
		UploadProtocol::reply(recvFD, UploadProtocol::STATUS_ERROR);
		close(recvFD);
		return;
	}*/
}
void UploadManager::stop() {
	std::cout << "UploadManager requested to stop" << std::endl;
	keepRunning = false;
}


} /* namespace upload */

