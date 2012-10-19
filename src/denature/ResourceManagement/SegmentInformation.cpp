/*
 * SegmentInformation.cpp
 *
 *  Created on: Oct 17, 2012
 *      Author: yuxiao
 */

#include "SegmentInformation.h"

SegmentInformation::SegmentInformation() {
	// TODO Auto-generated constructor stub

}

SegmentInformation::~SegmentInformation() {
	// TODO Auto-generated destructor stub
	std::list<StreamInformation*>::iterator list_it;

	for(list_it = streamList.begin(); list_it!= streamList.end(); list_it++)
	{
			delete (*list_it);
	}
}

void SegmentInformation::addToList(StreamInformation* newStream){

	streamList.push_back(newStream);
}

StreamInformation* SegmentInformation::getByOutputType(STREAM_OUTPUT_TYPE type){

	std::list<StreamInformation*>::iterator list_it;

	for(list_it = streamList.begin(); list_it!= streamList.end(); list_it++)
	{
		if((*list_it)->getOutputType() == type)
			return *list_it;
	}

	return NULL;
}


StreamInformation* SegmentInformation::getByID(const std::string id){

	std::list<StreamInformation*>::iterator list_it;

	for(list_it = streamList.begin(); list_it!= streamList.end(); list_it++)
	{
		if((*list_it)->getStreamID().compare(id) == 0)
			return *list_it;
	}

	return NULL;
}

//remove stream
bool SegmentInformation::removeFromList(const std::string id){

	std::list<StreamInformation*>::iterator list_it;

	for(list_it = streamList.begin(); list_it!= streamList.end(); list_it++)
	{
		if((*list_it)->getStreamID().compare(id) == 0){
			streamList.erase(list_it);
			return true;
		}
	}

	return false;
}


void SegmentInformation::setSegmentID(const std::string s){

	id = s;
}

//delete the old stream info
void SegmentInformation::reset(){


	while (!streamList.empty())
		streamList.pop_back();

}

void SegmentInformation::setSourceType(STREAM_SOURCE_TYPE t){

	sourceType = t;
}
