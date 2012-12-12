/*
 * MP4FileUpload.cpp
 *
 *  Created on: Oct 12, 2012
 *      Author: pieter
 */

#include "MP4FileUpload.h"
#include <iostream>
#include <sys/socket.h>
#include <errno.h>
#include <cstring>
#include "UploadProtocol.h"
#include "SocketUtils.h"
#include <fstream>

namespace upload {

//input s is /segment/segmentID/streamID/
void MP4FileUpload::finishing(std::string s) {


	//inform decoding thread
	SegmentInformation* newSegment = new SegmentInformation();
	newSegment->setSegmentID(s.substr(0, (s.find_first_of("/", 9) +1)));
	newSegment->setSourceType(TYPE_RECORDED);


	StreamInformation* newStream = new StreamInformation();

	newStream->setStreamID(s);

	std::string filePath = s.substr((s.find_first_of("/", 9)+1), (s.find_last_of("/", s.size())-s.find_first_of("/", 9)-1));

	newStream->setLocalFilePath(filePath);

	newStream->setOutputType(TYPE_UNKNOWN);

	newStream->setStatus(PROCESSING);

	newSegment->setIndicator(FILE_UPLOADED);

	newSegment->addToList(newStream);

	s_queue->push(newSegment);

}

void MP4FileUpload::stop() {
	keepRunning = false;
}


} /* namespace upload */
