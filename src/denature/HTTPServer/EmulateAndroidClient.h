/*
 * EmulateAndroidClient.h
 *
 *  Created on: Dec 5, 2012
 *      Author: yuxiao
 */

#ifndef EMULATEANDROIDCLIENT_H_
#define EMULATEANDROIDCLIENT_H_
#include <boost/filesystem.hpp>
#include <jansson.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <ostream>
#include <iostream>
#include <fstream>

using boost::asio::ip::tcp;

class EmulateAndroidClient {
public:
	EmulateAndroidClient();
	virtual ~EmulateAndroidClient();

	void operator()();
	bool createSegReq();
	void loadConfiguration();
	bool createStreamReq();
	bool updateFileInfo();
	void setIteration(int i);


	std::string user_id;

	std::string host_;
	int port_;
	std::string segID;
	std::string streamID;
	int index;
	int uploadInterval;
	std::string folder;
	int rounds;
};

#endif /* EMULATEANDROIDCLIENT_H_ */
