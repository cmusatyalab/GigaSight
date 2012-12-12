/*
 * StreamInformation.cpp
 *
 *  Created on: Oct 16, 2012
 *      Author: yuxiao
 */

#include "StreamInformation.h"

StreamInformation::StreamInformation() {
	// TODO Auto-generated constructor stub


}

StreamInformation::~StreamInformation() {
	// TODO Auto-generated destructor stub
}

StreamInformation::StreamInformation(const std::string segID, STREAM_OUTPUT_TYPE t2){

	segment_id = segID;

	outputType = t2;
	status = INITIATED;
	currentCount = 0;
	lastUpdatedCount = 0;

	sFSPath = "";
	sLocalPath = "";
	duration = 0;
	startTimestamp = "";

}

void StreamInformation::addTag(std::string& tag){
	tags.push_back(tag);
}

void StreamInformation::setStreamID(std::string& id){

	stream_id = id;
}


std::string StreamInformation::getStreamID(){

	return stream_id;

}

void StreamInformation::setStatus(PROCESSING_STATUS s){

	status = s;
}

PROCESSING_STATUS StreamInformation::getStatus(){
	return status;
}




STREAM_OUTPUT_TYPE StreamInformation::getOutputType(){
	return outputType;
}



void StreamInformation::setOutputType(STREAM_OUTPUT_TYPE t){
	outputType = t;
}

std::string StreamInformation::getFilePath(){
	return sFSPath;
	
}

std::string StreamInformation::getLocalFilePath(){
	return sLocalPath;

}


void StreamInformation::setVideoInfo(double fps, cv::Size size, int codec){

	frameRate = fps;
	frameSize = size;
	codecCode = codec;
	
}

double StreamInformation::getVideoFrameRate(){
	return frameRate;
}


cv::Size StreamInformation::getVideoFrameSize(){
	return frameSize;
}


int StreamInformation::getInputVideoCodec(){
	return codecCode;
}

void StreamInformation::setFilePath(const std::string s){
	sFSPath.assign(s.c_str());
}

void StreamInformation::setLocalFilePath(std::string s){
	sLocalPath.assign(s.c_str());
}


void StreamInformation::setSegmentID(const std::string s){
	segment_id.assign(s.c_str());

}


std::string StreamInformation::getSegmentID(){
	return segment_id;
}


void StreamInformation::setDuration(const double d){
	duration = d;

}

double StreamInformation::getDuration(){
	return duration;
}


void StreamInformation::setStartTime(const std::string s){
	startTimestamp.assign(s.c_str());
}

std::string StreamInformation::getStartTime(){
	return startTimestamp;
}

int StreamInformation::getFrameCount(){
	return frameCount;
}

void StreamInformation::setFrameCount(const int i){
	frameCount = i;
}
