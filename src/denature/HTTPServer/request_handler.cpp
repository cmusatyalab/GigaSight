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


void request_handler::configure(const std::string host, const std::string port, SharedQueue<SegmentInformation*>* s, ProcessingRuleContainer* p){

	//newSegment = s;
	segmentList = s;
	ruleManager = p;

	host_ = host;
	port_ = atoi(port.c_str());

	bE2EPerfTest = false;
}

void request_handler::handle_request(const request& req, reply& rep) {

	std::cout << "handle_request" << std::endl;
	// Decode url to path.
	std::string request_path;
	if (!url_decode(req.uri, request_path)) {
		std::cout << "return bad request to mobile" << std::endl;
		rep = reply::stock_reply(reply::bad_request);
		return;
	}

	//check if this is a GET request
	if (boost::iequals(req.method, "GET")) {
		return handle_get(req, rep);
	}

	if (boost::iequals(req.method, "POST")) {
	   // std::cout << "receive post" << std::endl;
		return handle_post(req, rep);
	}

	if (boost::iequals(req.method, "PUT")) {
		//std::cout << "receive PUT" << std::endl;
		return handle_put(req,rep);
	}

	rep = reply::stock_reply(reply::bad_request);


}

int request_handler::getSourceType(const request& req){

	int typeCode = -1;

	json_error_t err_t;
	json_t* root = json_loads(req.body.c_str(), 0, &err_t);

	if (root == NULL){
		return typeCode;
	}

	json_t* container = json_object_get(root, "container");
	const char* source;

	if ((container != NULL) && json_is_string(container)){
		source = json_string_value(container);

		if ((boost::iequals(source, "MP4")) || (boost::iequals(source, "RTSP_RTP"))){
				typeCode = 0;
		} else if (boost::iequals(source, "GPS")){
				typeCode = 2;
		} else
				typeCode = -1;
	}

	return typeCode;
}

void request_handler::handle_post(const request& req, reply &rep) {

	//std::cout << "received /segment" << req.body << std::endl;

	SegmentInformation* newSegment;

	//receive POST from mobile
	if (boost::iequals(req.uri, "/segment")){
	


		json_error_t err_t;
		json_t* requestRoot = json_loads(req.body.c_str(), 0, &err_t);
		json_t* type;
		json_t* user;

		if (requestRoot != NULL){

			user = json_object_get(requestRoot, "user_id");
			type = json_object_get(requestRoot, "type");
		}

		const char* user_id;
		if (user != NULL)
			user_id = json_string_value(user);
		else
			user_id = "";


		//std::cout << "user_id read from mobile request " << user_id << std::endl;
		std::string result;

		Client c;

		if (!c.requestForSegment(host_, port_, user_id, result)){
			rep.status = reply::service_unavailable;
			rep.headers.resize(1);
			rep.headers[0].name = "Description";
			rep.headers[0].value = "Data manager does not respond";
			return;
		}


		//result will request_handler::look like "/api/dm/segment/1"

		rep.status = reply::created;
		rep.headers.resize(1);
		rep.headers[0].name = "Location";
		//if result is "/api/dm/segment/1"
		std::string segID = result.substr(7, result.length()-7);
		//segID here will be /segment/1/
		rep.headers[0].value = segID;

		//std::cout << segID << std::endl,

		newSegment = new SegmentInformation();

		newSegment->setSegmentID(segID);


		std::string sourceType;

		if (type != NULL){

			sourceType = json_string_value(type);

			//the source type is by default TYPE_NOT_SPECIFIED
			if (boost::iequals(sourceType, "recorded"))
				newSegment->setSourceType(TYPE_RECORDED);
			else
				newSegment->setSourceType(TYPE_LIVESTREAM);
		}


		segmentMap.insert(std::pair<std::string, SegmentInformation*>(segID, newSegment));

		//std::cout << "segmentMap size" << segmentMap.size() << std::endl;
		std::cout << "segmentID: " << segID << std::endl;

		return;

	}else if (boost::iequals(req.uri.substr(0, 9), "/segment/")){


		std::string uri;
		std::string path;

		//0: recorded or live stream video 1: GPS
		int typeCode = getSourceType(req);

		if (-1 == typeCode){
			rep.status = reply::bad_request;
			return;
		}


		std::map<std::string, SegmentInformation*>::iterator it;

		//std::cout << "search for " << req.uri << std::endl;

		//use /segment/segmentID/ as key, not segmentID
		it = segmentMap.find(req.uri);

		if (it != segmentMap.end()){

			if (NULL == (*it).second)
				return;

			//we assume that the request for GPS stream will be sent first
		}
		else {

			std::cout << "Segment ID does not exist" << std::endl;
			rep.status = reply::bad_request;
			return;
		}


		Client c;

		if (!c.requestForStream(host_, port_, req, typeCode, uri, path)){
			rep.status = reply::service_unavailable;
			rep.headers.resize(1);
			rep.headers[0].name = "Description";
			rep.headers[0].value = "Data manager does not respond";
			return;
		}


		rep.status = reply::created;
		rep.headers.resize(1);
		rep.headers[0].name = "Location";
		//uri looks like "/api/dm/stream/10/"; location = /segment/segmentID/streamID/
		rep.headers[0].value = req.uri + uri.substr(15, uri.size()-15);


		StreamInformation* oriStream;

		if (typeCode == 0)
			oriStream = new StreamInformation(req.uri, TYPE_ORIGINAL_VIDEO);
		else
			oriStream = new StreamInformation(req.uri, TYPE_GPS_STREAM);

		//uri here is still /api/dm/stream/streamID/


		std::string temp = req.uri + uri.substr(15, (uri.size()-15));

		oriStream->setFilePath(path);
		std::cout << "original video/gps" << uri << " should be saved to: " << oriStream->getFilePath() << std::endl;
		oriStream->setStreamID(temp);
		oriStream->setSegmentID(req.uri);
		oriStream->setStatus(WAITING_FOR_INPUT);
		(*it).second->addToList(oriStream);

		//std::cout << "stream ID" << temp << std::endl;

		if (typeCode == 2){

			(*it).second->setGPSIncluded();
			std::cout << "this segment includes gps" << std::endl;
			return;

		}

		if (!c.requestForStream(host_, port_, req, 1, uri, path)){

			rep.status = reply::service_unavailable;
			rep.headers.resize(1);
			rep.headers[0].name = "Description";
			rep.headers[0].value = "Data manager does not respond";
			return;
		}


		temp = req.uri + uri.substr(15, (uri.size()-15));


		//uri here is still /api/dm/stream/streamID
		StreamInformation* deVideo = new StreamInformation(req.uri, TYPE_DENATURED_VIDEO);
		deVideo->setFilePath(path);
		deVideo->setStreamID(temp);
		deVideo->setSegmentID(req.uri);
		deVideo->setStatus(WAITING_FOR_INPUT);
		deVideo->setOutputType(TYPE_DENATURED_VIDEO);
		(*it).second->addToList(deVideo);

		//std::cout << "denatured video" << uri << "should be: " << deVideo->getFilePath() << std::endl;

		(*it).second->setVideoInfoReceived();

		newSegment = (*it).second;

		if (newSegment->getSourceType() == TYPE_LIVESTREAM){

			if (newSegment->isGPSIncluded())
				newSegment->setIndicator(NEW_LIVE_STREAMING_GPS);
			else
				newSegment->setIndicator(NEW_LIVE_STREAMING_ONLY);

			newSegment->setVideoInfoReceived();

			segmentList->push(newSegment);

			if (!newSegment->isGPSIncluded()){
				segmentMap.erase(it);
				std::cout << "delete record " <<std::endl;
			}

			return;
		}

		//if it is recorded videos, wait until PUT is sent, and then push the segmentinfo to the queue


		//for testing (if the video uploads stops due to bad network conditions and then resume after a while.
		//we should allow the video decoding and processing to start if GPS stream is also available

		if (newSegment->isGPSIncluded())
			newSegment->setIndicator(NEW_RECORDED_GPS);
		else
			newSegment->setIndicator(NEW_RECORDED);


		segmentList->push(newSegment);
		//testing ends here
		//std::cout << "stream segmentMap size " << segmentMap.size() << std::endl;

		return;

	}else if (req.uri.find("/privacy") != req.uri.npos){

		//parse req.body and edit processingrulecontainer
		//mobile always sends a POST first, and then PUT
		ruleManager->resetAllRules();

		rep.headers.resize(1);
		rep.headers[0].name = "Location";
		rep.headers[0].value = "/privacy/1";
		rep.status = reply::created;
		return;

}


	//if the url is wrong
	rep = reply::stock_reply(reply::bad_request);

}


void request_handler::handle_put(const request& req, reply &rep) {


	if (req.uri.find("/privacy") != req.uri.npos){

			//parse req.body and edit processingrulecontainer
			//remove all the old rules?
			ruleManager->resetAllRules();

			//std::cout << req.body << std::endl;
			parseRuleFromReq(req);

			rep.headers.resize(1);
			rep.headers[0].name = "Location";
			rep.headers[0].value = "/privacy/1";
			rep.status = reply::created;
			return;

	}


	if (req.uri.find("/segment/") != req.uri.npos){

		//req.uri is /segment/segmentID/streamID/
		//std::cout << req.body << std::endl;

		//std::cout << "handle PUT" << std::endl;

		std::map<std::string, SegmentInformation*>::iterator it;

		//use /segment/segmentID/ as key, not segmentID
		std::string segID = req.uri.substr(0, req.uri.find_last_of("/", req.uri.find_last_of("/")-1))+"/";
		//std::cout << "UPDATE_FILE_INFO: segment ID " << segID << std::endl;


		it = segmentMap.find(segID);

		if (it == segmentMap.end()){
			std::cout << "Segment ID" << segID << " does not exist" << std::endl;
			rep.status = reply::bad_request;
			return;
		}


		json_error_t err_t;
		json_t* root = json_loads(req.body.c_str(), 0, &err_t);

		if (root == NULL){
				rep.status = reply::bad_request;
				return;
		}

		json_t* duration = json_object_get(root, "duration");

		json_t* timestamp = json_object_get(root, "startTime");


		SegmentInformation* newSegment = (*it).second;

		if (newSegment != NULL){

				StreamInformation* newStream = newSegment->getByStreamID(req.uri);


				if ((newStream != NULL) && (duration != NULL) && json_is_integer(duration)){
					newStream->setDuration(json_integer_value(duration)/1000);//convert from ms to s
					//std::cout << "new stream duration " << newStream->getDuration() << std::endl;
				}


				if ((newStream != NULL) && (timestamp != NULL) && json_is_string(timestamp)){

					newStream->setStartTime(json_string_value(timestamp));
					//std::cout << "new stream start time " << newStream->getStartTime() << std::endl;
				}

				Client c;
				c.updateStreamInformation(host_, port_, req.uri, req.body);

				if (newStream->getOutputType() == TYPE_ORIGINAL_VIDEO){

					(*it).second->setVideoDurationAvailable();

					//the following line is just for end-to-end testing
					if (bE2EPerfTest)
						(*it).second->setVideoAvailable();

					if ((*it).second->isGPSIncluded() && (*it).second->isGPSInfoReceived()){
							newSegment->setIndicator(UPDATE_FILE_INFO);
							segmentList->push(newSegment);
							segmentMap.erase(it);
					}
					else if (!(*it).second->isGPSIncluded()){
						newSegment->setIndicator(UPDATE_FILE_INFO);
						segmentList->push(newSegment);
						segmentMap.erase(it);
					}


				} else if (newStream->getOutputType() == TYPE_GPS_STREAM){

					(*it).second->setGPSInfoReceived();

					if (((*it).second->isVideoInfoReceived()) && (*it).second->isVideoDurationAvailable()){
						newSegment->setIndicator(UPDATE_FILE_INFO);
						segmentList->push(newSegment);
						segmentMap.erase(it);
					}

				}//outputtype
				else
					std::cout << "unknown type" << std::endl;

		}//newsegment != null

		rep = reply::stock_reply(reply::ok);

	}

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

	reply::stock_reply(reply::not_found);
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

void request_handler::parseRuleFromReq(const request& req){

	json_error_t err_t;
	json_t* root = json_loads(req.body.c_str(), 0, &err_t);
	json_t* rules;

	if (root == NULL){
		std::cout << req.body  << std::endl;
		std::cout << err_t.text << std::endl;
		return;
	}

//	std::cout << "parse rule " << req.body << std::endl;

	rules = json_object_get(root, "rules");

	if (rules == NULL)
			return;


	if (json_is_array(rules)){

		for (unsigned int j=0; j<json_array_size(rules); j++)
			addOneRule(json_array_get(rules, j));

	}else{

		addOneRule(rules);
	}

	std::cout << "rule list size " << ruleManager->getRuleCount() << std::endl;
}


void request_handler::addLocationRule(json_t* params, PROCESSING_RULE& rule){

	json_t* param;
	size_t size = 1;
	bool bArray = false;

	if (json_is_array(params)){
		size = json_array_size(params);
		bArray = true;

	}


	param = params;


	for (unsigned int j=0; j < size; j++){

		LOCATION_FILTER locationFilter;

		locationFilter.setType(LOCATION);

		if (bArray)
			param = json_array_get(params, j);

		//radius, latitude, longitude
		if (json_object_size(param) == 3){

			json_t* radius = json_object_get(param, "radius");

			if ((radius != NULL) && json_is_real(radius))
				locationFilter.addParam(json_real_value(radius));

			json_t* lat = json_object_get(param, "centre_lat");

			if ((lat != NULL) && json_is_real(lat))
				locationFilter.addParam(json_real_value(lat));

			json_t* lon = json_object_get(param, "centre_long");

			if ((lon != NULL) && json_is_real(lon))
				locationFilter.addParam(json_real_value(lon));

			rule.locationFilterList.push_back(locationFilter);

		} else if (json_object_size(param) == 4){

			json_t* lat_down = json_object_get(param, "lat_downleft");

			if ((lat_down != NULL) && json_is_real(lat_down))
				locationFilter.addParam(json_real_value(lat_down));

			json_t* long_down = json_object_get(param, "long_downleft");

			if ((long_down != NULL) && json_is_real(long_down));
				locationFilter.addParam(json_real_value(long_down));

			json_t* lat_up = json_object_get(param, "lat_upright");

			if ((lat_up != NULL) && json_is_real(lat_up))
				locationFilter.addParam(json_real_value(lat_up));

			json_t* long_up = json_object_get(param, "long_upright");

			if ((long_up != NULL) && json_is_real(long_up))
				locationFilter.addParam(json_real_value(long_up));

			rule.locationFilterList.push_back(locationFilter);

		}//if

	}//for

	std::cout << "a location rule added" << std::endl;

}


void request_handler::addTimeRule(json_t* params, PROCESSING_RULE& rule){

	json_t* param;
	size_t size =1;
	bool bArray = false;

	if (json_is_array(params)){
		size = json_array_size(params);
		bArray = true;
	}

	std::cout << "Add time rule with " << size << "params" << std::endl;

	param = params;

	for (unsigned int j=0; j < size; j++){

		TIME_FILTER timeFilter;

		timeFilter.setType(TIMEFRAME);

		if (bArray)
			param = json_array_get(params, j);

		if (param == NULL)
			continue;

		json_t* time_start = json_object_get(param, "time_start");
		json_t* time_stop = json_object_get(param, "time_stop");

		if ((time_start != NULL) && (time_stop != NULL)){

				std::string startStr = json_string_value(time_start);
				std::string endStr = json_string_value(time_stop);

				struct timepoint start, end;

				start.hour = atoi(startStr.substr(0, startStr.find_last_of("-")).c_str());
				start.second = 0;

				//HH:MM::SS
				if (startStr.find_last_of(":") != startStr.find(":")){
						start.min = atoi(startStr.substr(startStr.find(":")+1, startStr.find_last_of(":")-startStr.find(":")-1).c_str());
						start.second = atoi(startStr.substr(startStr.find_last_of(":")+1, startStr.size()-startStr.find_last_of(":")-1).c_str());
				}
				else
						start.min = atoi(startStr.substr(startStr.find_last_of(":")+1, startStr.size()-startStr.find_last_of(":")-1).c_str());


				end.hour = atoi(endStr.substr(0, endStr.find_last_of("-")).c_str());

				//HH:MM::SS
				if (endStr.find_last_of(":") != endStr.find(":")){
						end.min = atoi(endStr.substr(startStr.find(":")+1, endStr.find_last_of(":")-endStr.find(":")-1).c_str());
						end.second = atoi(endStr.substr(endStr.find_last_of(":")+1, endStr.size()-endStr.find_last_of(":")-1).c_str());
				}
				else
						end.min = atoi(endStr.substr(endStr.find_last_of(":")+1, endStr.size()-endStr.find_last_of(":")-1).c_str());


				std::cout << "Time filter from " << start.hour << ":" << start.min<< "to" << end.hour << ":" << end.min << std::endl;

				timeFilter.addParam(std::pair<timepoint, timepoint>(start, end));
				rule.timeFilterList.push_back(timeFilter);

		}
	}//for

}


void request_handler::addContentRule(json_t* params, PROCESSING_RULE& rule){

	std::cout << "CONTENT RULE" << rule.actionType << std::endl;

	PrivacyFilter<std::string> contentFilter;

	contentFilter.setType(CONTENT);

	if (params == NULL)
		return;

	json_t* param;

	if (json_is_array(params)){

		for (unsigned int j=0; j < json_array_size(params); j++){

			param = json_array_get(params, j);

			if ((param != NULL) && json_is_string(param))

				contentFilter.addParam(json_string_value(param));

				ruleManager->addToNameList(json_string_value(param));
			}

			std::cout << "These persons' faces will be blurred: " << ruleManager->getNames()  << std::endl;


	}else{

	   if (json_is_string(params)){

		   contentFilter.addParam(json_string_value(params));
		   ruleManager->addToNameList(json_string_value(params));
	   }

		std::cout << "contentFilter param" << ruleManager->getNames()  << std::endl;
	}//else

	rule.faceFilterList.push_back(contentFilter);

}


void request_handler::addOneRule(json_t* rules){

	PROCESSING_RULE newRule;

	std::cout << "A NEW RULE" << std::endl;

	json_t *action = json_object_get(rules, "action");

	if (action == NULL)
			return;


	//clarify it later
	if (boost::iequals("blank", json_string_value(action))){
			newRule.actionType = DELETE_FRAME;//will change to blank later
	} else if (boost::iequals("blur", json_string_value(action))){
			std::cout << "action: " << json_string_value(action) << std::endl;
			newRule.actionType = REMOVE_FACE;
	} else if (boost::iequals("publish", json_string_value(action))){
			newRule.actionType = NO_ACTION;
	}

	newRule.index = 0;

	json_t *conditions = json_object_get(rules, "conditions");

	if (conditions == NULL){
		std::cout << "conditions empty" << std::endl;
		return;
	}

	size_t condSize = 1;
	bool bArray = false;

	if (json_is_array(conditions)){
		condSize = json_array_size(conditions);
		bArray = true;
	}

	json_t* condition;

	if (!bArray){
		condition = conditions;
	}

	for (unsigned int i=0; i< condSize; i++){

			if (bArray)
				condition = json_array_get(conditions, i);

			if (condition == NULL){
				std::cout << "condition is NULL " << std::endl;
				continue;
			}

			json_t* type = json_object_get(condition, "type");

			if (type == NULL)
				continue;

			//std::cout << "type" << json_string_value(type) << std::endl;

			json_t* params = json_object_get(condition, "data");

			if (params == NULL)
				continue;


			if (boost::iequals(json_string_value(type), "location")){

				addLocationRule(params, newRule);

			} else if (boost::iequals(json_string_value(type), "timeframe")){

				addTimeRule(params, newRule);

			} else if (boost::iequals(json_string_value(type), "content")){

				addContentRule(params, newRule);

			}

	}//for

  	ruleManager->addRule(newRule);

}



} // namespace server
} // namespace http
