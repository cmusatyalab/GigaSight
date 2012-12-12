//
// request_handler.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//revisions made into handle_post()

#include "request_handler.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include "mime_types.hpp"
#include "reply.hpp"
#include "request.hpp"
#include "Client.h"
#include "../ResourceManagement/StreamInformation.h"
#include <jansson.h>

#ifdef __CDT_PARSER__
#undef BOOST_FOREACH
#define BOOST_FOREACH(a, b) for(a; ; )
#endif


namespace http {
namespace server {

//prm::PrivateResourceManager& resMan = prm::PrivateResourceManager::Instance();
void request_handler::configure(SegmentInformation* s, ProcessingRuleContainer* p){

	newSegment = s;
	ruleManager = p;
	//for testing
	//consider loading from configure file
	host_ = "128.2.213.15";
	port_ = 5000;
	user_id = "1";
}

void request_handler::handle_request(const request& req, reply& rep) {

	std::cout << "handle_request" << std::endl;
	// Decode url to path.
	std::string request_path;
	if (!url_decode(req.uri, request_path)) {
		rep = reply::stock_reply(reply::bad_request);
		return;
	}

	//check if this is a GET request
	if (boost::iequals(req.method, "GET")) {
		return handle_get(req, rep);
	}

	if (boost::iequals(req.method, "POST")) {
	    std::cout << "receive post" << std::endl;
		return handle_post(req, rep);
	}

	if (boost::iequals(req.method, "PUT")) {
		return handle_put(req,rep);
	}

	rep = reply::stock_reply(reply::bad_request);
}


void request_handler::handle_post(const request& req, reply &rep) {

	//receive POST from mobile
	if (boost::iequals(req.uri, "/segment")){
	
		std::cout << "received /segment" << req.body;

		std::string result;
		Client c;
		//where to get user_id?
		c.requestForSegment(host_, port_, user_id, result);

		//result will look like "/api/dm/segment/1"

		rep.status = reply::created;
		rep.headers.resize(1);
		rep.headers[0].name = "Location";
		//value should be "/segment/1"
		std::string segID = result.substr(7, result.length()-7);
		rep.headers[0].value = segID;

		newSegment->setSegmentID(segID);

		newSegment->reset();

		json_error_t err_t;
		json_t* responseRoot = json_loads(req.body.c_str(), 0, &err_t);
		json_t* type;

		if (responseRoot != NULL){

				type = json_object_get(responseRoot, "type");
		}


		std::string sourceType;
		if (type != NULL){

			sourceType = json_string_value(type);

			if (boost::iequals(sourceType, "recorded"))
				newSegment->setSourceType(TYPE_RECORDED);
			else
				newSegment->setSourceType(TYPE_LIVESTREAM);
		}
		else
			newSegment->setSourceType(TYPE_NOT_SPECIFIED);

		return;

	}else if (boost::iequals(req.uri, "/privacy")){
		
		//parse req.body and edit processingrulecontainer

		parseRuleFromReq(req.body);

		rep = reply::stock_reply(reply::ok);

	}else if (boost::iequals(req.uri.substr(0, 9), "/segment/")){

		std::string uri;
		std::string path;

		Client c;
		c.requestForStream(host_, port_, req, 0, uri, path);

		rep.status = reply::created;
		rep.headers.resize(1);
		rep.headers[0].name = "Location";
		rep.headers[0].value = uri;

		StreamInformation* oriVideo = new StreamInformation(req.uri, TYPE_ORIGINAL_VIDEO);
		oriVideo->setFilePath(path);
		oriVideo->setStreamID(uri);
		oriVideo->setStatus(INITIATED);
		newSegment->addToList(oriVideo);

		c.requestForStream(host_, port_, req, 1, uri, path);

		StreamInformation* deVideo = new StreamInformation(req.uri, TYPE_DENATURED_VIDEO);
		deVideo->setFilePath(path);
		deVideo->setStreamID(uri);
		deVideo->setStatus(WAITING_FOR_INPUT);
		newSegment->addToList(deVideo);

		//inform VideoDecoding thread to start receiving data

		return;
	}

/*
	prm::pResource_t pRes;

	prm::ResourceURI_t resURI(req.uri);
	prm::Status stat = resMan.create(pRes, resURI, req.body);

	switch (stat) {
	case prm::ok :
		rep.status = reply::created;
		rep.headers.resize(1);
		rep.headers[0].name = "Location";
		rep.headers[0].value = pRes->getLocation();
		return;

	case prm::not_found :
		rep = reply::stock_reply(reply::not_found);
		return;

	case prm::bad_request :
		rep = reply::stock_reply(reply::bad_request);
		return;
	}*/

}

void request_handler::handle_put(const request& req, reply &rep) {

/*
	prm::ResourceURI_t resURI(req.uri);
	prm::Status stat = resMan.update(resURI, req.body);

	switch (stat) {
	case prm::ok :
		rep = reply::stock_reply(reply::ok);
		return;

	case prm::not_found :
		rep = reply::stock_reply(reply::not_found);
		return;

	case prm::bad_request :
		rep = reply::stock_reply(reply::bad_request);
		return;
	}*/
}


void request_handler::handle_get(const request& req, reply& rep) {

	//for testing

/*
	prm::pResource_t pRes;

	prm::ResourceURI_t resURI(req.uri);
	prm::Status stat = resMan.read(resURI, pRes);
	std::string data;
	switch(stat) {
	case prm::ok :
		data = pRes->toJSON();
		rep.content.append(data);
		rep.status = reply::ok;
		rep.headers.resize(2);
		rep.headers[0].name = "Content-Type";
		rep.headers[0].value = "application/json";
		rep.headers[1].name = "Content-Length";
		rep.headers[1].value = boost::lexical_cast<std::string>(rep.content.size());
		return;
	case prm::not_found :
		rep = reply::stock_reply(reply::not_found);
		return;
	case prm::bad_request :
		rep = reply::stock_reply(reply::bad_request);
		return;
	}
	*/
}

bool request_handler::url_decode(const std::string& in, std::string& out) {

	out.clear();
	out.reserve(in.size());

	for (std::size_t i = 0; i < in.size(); ++i) {
		if (in[i] == '%') {
			if (i + 3 <= in.size()) {
				int value = 0;
				std::istringstream is(in.substr(i + 1, 2));
				if (is >> std::hex >> value) {
					out += static_cast<char>(value);
					i += 2;
				} else {
					return false;
				}
			} else {
				return false;
			}
		} else if (in[i] == '+') {
			out += ' ';
		} else {
			out += in[i];
		}
	}//for

	return true;
}

void request_handler::parseRuleFromReq(std::string req){

	json_error_t err_t;
	json_t* root = json_loads(req.c_str(), 0, &err_t);
	json_t* rules;

	if (root == NULL)
		return;


	rules = json_object_get(root, "rules");

	if (rules == NULL)
			return;


	if (json_is_array(rules)){

		for (int j=0; j<json_array_size(rules); j++)
			addOneRule(json_array_get(rules, j));


	}else{
		addOneRule(rules);
	}

}



void request_handler::addOneRule(json_t* rules){


	json_t *action = json_object_get(rules, "action");

	if (action == NULL)
				return;

	std::cout << "action" << json_string_value(action) << std::endl;

	json_t *conditions = json_object_get(rules, "conditions");

	PROCESSING_RULE newRule;

	if (json_is_array(conditions)){


		for (int i=0; i< json_array_size(conditions); i++){

			json_t * condition = json_array_get(conditions, i);

			if (condition == NULL)
				break;

			json_t* type = json_object_get(condition, "type");

			if (type == NULL)
				continue;

			if (boost::iequals(json_string_value(type), "location")){

				PrivacyFilter<double> locationFilter;

				locationFilter.setType(LOCATION);

				json_t* params = json_object_get(condition, "data");

				if (params == NULL)
					continue;

				if (json_is_array(params)){

						for (int j=0; j < json_array_size(params); j++)
							locationFilter.addParam(atof(json_string_value(json_array_get(params, j))));


				}else{
						locationFilter.addParam(atof(json_string_value(params)));
				}

					newRule.locationFilterList.push_back(locationFilter);
				}
			else if (boost::iequals(json_string_value(type), "timeframe")){


			}else if (boost::iequals(json_string_value(type), "content")){

					PrivacyFilter<std::string> contentFilter;
					contentFilter.setType(CONTENT);

					json_t* params = json_object_get(condition, "data");

					if (params == NULL)
						continue;

					if (json_is_array(params)){

						for (int j=0; j < json_array_size(params); j++)
							contentFilter.addParam(json_string_value(json_array_get(params, j)));


					}else{
						contentFilter.addParam(json_string_value(params));
					}

					newRule.faceFilterList.push_back(contentFilter);
				}

			}//for

		}//if
		ruleManager->addRule(newRule);
}

} // namespace server
} // namespace http
