/*
 * SocketUtils.cpp
 *
 *  Created on: Oct 13, 2012
 *      Author: Pieter Simoens
 */


#include "SocketUtils.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <boost/lexical_cast.hpp>
#include <cstring>
#include <errno.h>
#include <iostream>
//This method reads in exactly noBytes from the specified FD.
//It returns -1 on error, 0 if FD was closed by client or noBytes on success
int recvExact(int fd, char* buf, int noBytes) {

	int offset = 0;
	int ret = 0;
	while (offset < noBytes) {
		ret = recv(fd, buf + offset, noBytes - offset, 0);
		if (ret <= 0) {
			return ret;
		}
		offset += ret;
	}
	return ret;
}

int openListenSocket(int listenPort) {
	int listenFD = 0;
	struct addrinfo hints, *servinfo, *p;
	int yes = 1;
	char s[INET6_ADDRSTRLEN];
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // listen on all available IP interfaces

	std::string portString = boost::lexical_cast<std::string>(listenPort);
	if ((rv = getaddrinfo(NULL, portString.c_str(), &hints, &servinfo)) != 0) {
		std::cerr << "getaddrinfo: %s\n" << gai_strerror(rv) << std::endl;
		return -1;
	}

	// loop through all the results and bind to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((listenFD = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
				== -1) {
			std::cerr << "socket" << strerror(errno) << std::endl;
			continue;
		}

		if (setsockopt(listenFD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
				== -1) {
			std::cerr << "setsockopt" << strerror(errno) << std::endl;
			freeaddrinfo(servinfo);
			close(listenFD);
			return -1;
		}

		if (bind(listenFD, p->ai_addr, p->ai_addrlen) == -1) {
			std::cerr << "bind" << strerror(errno) << std::endl;
			continue;
		}
		break;
	}

	if (p == NULL) {
		std::cerr << "server: failed to bind" << std::endl;
		close(listenFD);
		freeaddrinfo(servinfo);
		return -1;
	}

	//this will return 0.0.0.0 as we are accepting connections on ANY networkadapter...
	inet_ntop(AF_INET, &(((struct sockaddr_in *) (p->ai_addr))->sin_addr), s,
			sizeof s);


	freeaddrinfo(servinfo); // all done with this structure

	//maximum 10 simultaneous connections in queue
	if (listen(listenFD, 10) == -1) {
		std::cerr << "listen" << strerror(errno) << std::endl;
		close(listenFD);
		return -1;
	}

	return listenFD;
}
