/*
 * SegmentInformation.cpp
 *
 *  Created on: Oct 17, 2012
 *      Author: yuxiao
 */

#include "SegmentInformation.h"
#include <boost/algorithm/string.hpp>

SegmentInformation::SegmentInformation() {
	// TODO Auto-generated constructor stub
	sourceType = TYPE_NOT_SPECIFIED;
	bIncludeGPS = false;
	bGPSUploaded = false;
	indicator = NOT_SPECIFIED;
	bVideoUploaded = false;
	bGPSFileInfoReceived = false;
	bVideoFileInfoReceived = false;
	bVideoDurationAvailable = 0;

}

SegmentInformation::~SegmentInformation() {
	// TODO Auto-generated destructor stub
	std::map<STREAM_OUTPUT_TYPE, StreamInformation*>::iterator list_it;

	for(list_it = streamMap.begin(); list_it!= streamMap.end(); list_it++)
	{
			delete (*list_it).second;
	}
}

void SegmentInformation::addToList(StreamInformation* newStream){

	//streamList.push_back(newStream);
	if (newStream == NULL)
		return;

	if (newStream->getOutputType() == TYPE_UNKNOWN){

			tempMap.insert(std::pair<std::string, std::string>(newStream->getStreamID(), newStream->getLocalFilePath()));

			std::cout << "insert to tempMap " << newStream->getStreamID() << ":" << newStream->getLocalFilePath() << std::endl;

			return;
	}

	streamMap.insert(std::pair<STREAM_OUTPUT_TYPE, StreamInformation*>(newStream->getOutputType(), newStream));
}

std::string SegmentInformation::getStoredFilePath(const std::string streamID){

	std::map<std::string, std::string>::iterator it = tempMap.find(streamID);

	std::string result = "";
	if (it != tempMap.end()){
		result = (*it).second;
		return result;
	}
	else
		return "";

}


StreamInformation* SegmentInformation::getByOutputType(STREAM_OUTPUT_TYPE type){

	std::map<STREAM_OUTPUT_TYPE, StreamInformation*>::iterator it;

	it = streamMap.find(type);

	if (it != streamMap.end())
		return (*it).second;

	return NULL;
}


StreamInformation* SegmentInformation::getByStreamID(const std::string id){

	std::map<STREAM_OUTPUT_TYPE, StreamInformation*>::iterator it;

	for(it = streamMap.begin(); it!= streamMap.end(); it++)
	{
		//std::cout << "compare with " << (*it).second->getStreamID() << std::endl;
		if(boost::iequals((*it).second->getStreamID(), id)){
			//std::cout << "Find stream " << std::endl;
			return ((*it).second);
		}
	}

	return NULL;
}

//remove stream
bool SegmentInformation::removeFromList(const std::string id){


	std::map<STREAM_OUTPUT_TYPE, StreamInformation*>::iterator it;

		for(it = streamMap.begin(); it!= streamMap.end(); it++)
		{
			if(0 == (*it).second->getStreamID().compare(id)){
				streamMap.erase(it);
				return true;
			}
		}

		return false;
}

bool SegmentInformation::removeStreamByOutputType(STREAM_OUTPUT_TYPE type){


	std::map<STREAM_OUTPUT_TYPE, StreamInformation*>::iterator it;

	it = streamMap.find(type);

		if (it != streamMap.end()){

			streamMap.erase(it);
			return true;
		}

		return false;
}


void SegmentInformation::setSegmentID(const std::string s){

	id = s;
}


void SegmentInformation::setSourceType(STREAM_SOURCE_TYPE t){

	sourceType = t;
}

STREAM_SOURCE_TYPE SegmentInformation::getSourceType(){
	return sourceType;
}


std::string SegmentInformation::getSegmentID(){
	return id;
}



void SegmentInformation::setIndicator(TYPE_INDICATOR i){
	indicator = i;
}


TYPE_INDICATOR SegmentInformation::getIndicator(){

	return indicator;

}


void SegmentInformation::setGPSIncluded(){

	bIncludeGPS = true;
}


bool SegmentInformation::isGPSIncluded(){

	return bIncludeGPS;
}

void SegmentInformation::setGPSAvailable(){

	bGPSUploaded = true;
}


bool SegmentInformation::isGPSAvailable(){

	return bGPSUploaded;
}

StreamInformation* SegmentInformation::getFirstStream(){

	if (streamMap.empty())
		return NULL;

	std::map<STREAM_OUTPUT_TYPE, StreamInformation*>::iterator it = streamMap.begin();

	return (*it).second;
}


void SegmentInformation::setVideoAvailable(){
	bVideoUploaded = true;
}

bool SegmentInformation::isVideoAvailable(){
	return bVideoUploaded;
}

bool SegmentInformation::isVideoInfoReceived(){
	return bVideoFileInfoReceived;
}

bool SegmentInformation::isGPSInfoReceived(){

	return bGPSFileInfoReceived;
}

void SegmentInformation::setVideoInfoReceived(){

	bVideoFileInfoReceived = true;
}


void SegmentInformation::setGPSInfoReceived(){

	bGPSFileInfoReceived = true;
}

size_t SegmentInformation::getStreamCount(){

	return streamMap.size();

}

std::string SegmentInformation::getFirstStreamID(){

	if (tempMap.size() > 0){
		return (*tempMap.begin()).first;
	}
	else
		return "";
}


void SegmentInformation::addToTempList(std::string key, std::string value){

	tempMap.insert(std::pair<std::string, std::string> (key, value));
}



int SegmentInformation::isVideoDurationAvailable(){


	return bVideoDurationAvailable;

}

void SegmentInformation::setVideoDurationAvailable(){

	bVideoDurationAvailable = 1;
}


void SegmentInformation::setStatus(PROCESSING_STATUS s){

	status = s;
}

PROCESSING_STATUS SegmentInformation::getStatus(){
	return status;
}
