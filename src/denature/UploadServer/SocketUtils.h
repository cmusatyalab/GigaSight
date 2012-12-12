/*
 * SocketUtils.h
 *
 *  Created on: Oct 13, 2012
 *      Author: Pieter Simoens
 */

#ifndef SOCKETUTILS_H_
#define SOCKETUTILS_H_


//This method opens a listening socket on all interfaces of this machine
//return -1 or the FDof the listening socket
int openListenSocket(int listenPort);

//This method reads in exactly noBytes from the specified FD.
//It returns -1 on error, 0 if FD was closed by client or noBytes on success
int recvExact(int fd, char* buf, int noBytes);

#endif /* SOCKETUTILS_H_ */
