//
// request_handler.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_REQUEST_HANDLER_HPP
#define HTTP_REQUEST_HANDLER_HPP

#include <string>
#include <boost/noncopyable.hpp>

#include "../ResourceManagement/SegmentInformation.h"
#include "../ResourceManagement/ProcessingRuleContainer.h"
#include "../ImageProcessing/SharedQueue.h"
#include <iostream>
#include <istream>
#include <ostream>
#include <sstream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <jansson.h>

namespace http {
namespace server {

struct reply;
struct request;

/// The common handler for all incoming requests.
class request_handler: private boost::noncopyable {
public:

	/// Handle a request and produce a reply.
	void handle_request(const request& req, reply& rep);
	void configure(const std::string host, const std::string port, SharedQueue<SegmentInformation*>* s, ProcessingRuleContainer* p);

private:
	//SegmentInformation* newSegment;
	std::map<std::string, SegmentInformation*> segmentMap;
	SharedQueue<SegmentInformation*> *segmentList;

	ProcessingRuleContainer* ruleManager;

	//function called when mobile device wants to get the metadata of a resource
	void handle_get(const request& req, reply& rep);
	//function called when mobile device wants to create a new resource
	void handle_post(const request& req, reply& rep);
	//function called when mobile device wants to modify a server resource
	void handle_put(const request&req, reply& rep);
	/// Perform URL-decoding on a string. Returns false if the encoding was
	/// invalid.
	static bool url_decode(const std::string& in, std::string& out);

	int getSourceType(const request& req);

	std::string host_;
	int port_;
	bool bE2EPerfTest;

	void parseRuleFromReq(const request& req);
	void addOneRule(json_t* rules);
	void addLocationRule(json_t* params, PROCESSING_RULE& rule);
	void addTimeRule(json_t* params, PROCESSING_RULE& rule);
	void addContentRule(json_t* params, PROCESSING_RULE& rule);

};

} // namespace server
} // namespace http

#endif // HTTP_REQUEST_HANDLER_HPP
