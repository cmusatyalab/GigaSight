/*
 * UploadProtocol.h
 *
 *  Created on: Oct 12, 2012
 *      Author: pieter
 */

#ifndef UPLOADPROTOCOL_H_
#define UPLOADPROTOCOL_H_

#include <errno.h>
#include <iostream>
#include <arpa/inet.h>
namespace upload{

class UploadProtocol{
public:
enum e_status{
		STATUS_OK,
		STATUS_ERROR
	};

static void reply(int fd, int status){
	int nStatus = htonl(status);
	if(send(fd,&nStatus,sizeof(int),0) < 0){
		std::cerr << "Could not reply " << strerror(errno) << std::endl;
	}

}
};
}

#endif /* UPLOADPROTOCOL_H_ */
