/*
 * Client.h
 *
 *  Created on: Oct 18, 2012
 *      Author: yuxiao
 */

#ifndef CLIENT_H_
#define CLIENT_H_

#include <iostream>
#include <istream>
#include <ostream>
#include <sstream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <jansson.h>
#include "request.hpp"
#include "../ResourceManagement/StreamInformation.h"

using boost::asio::ip::tcp;

class Client {
public:
	virtual ~Client();
	Client();
	//return segmentID
	bool requestForSegment(const std::string& host_, const unsigned short port_, const std::string& user_id, std::string& result);

	//return streamID
	bool requestForStream(const std::string& host_, const unsigned short port_, const http::server::request& req, int type, std::string& uri, std::string& path);

	void updateProcessingStatus(const std::string& host_, const unsigned short port_, const std::string streamID, const PROCESSING_STATUS statusCode);

	void updateStreamInformation(const std::string& host_, const unsigned short port_, const std::string streamID, std::string body);

	void updateSegmentInformation(const std::string& host_, const unsigned short port_, const std::string segmentID, std::string body);
private:
/*	void handle_resolve(const boost::system::error_code& err,
	      tcp::resolver::iterator endpoint_iterator);

	void handle_connect(const boost::system::error_code& err,
	      tcp::resolver::iterator endpoint_iterator);

	void handle_write_request(const boost::system::error_code& err);

	void handle_read_status_line(const boost::system::error_code& err);

	void handle_read_headers(const boost::system::error_code& err);

	void handle_read_content(const boost::system::error_code& err);*/

	boost::asio::streambuf request_;
	boost::asio::streambuf response_;
};

#endif /* CLIENT_H_ */
