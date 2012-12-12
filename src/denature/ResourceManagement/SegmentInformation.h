/*
 * SegmentInformation.h
 *
 *  Created on: Oct 17, 2012
 *      Author: yuxiao
 */

#ifndef SEGMENTINFORMATION_H_
#define SEGMENTINFORMATION_H_
#include "StreamInformation.h"

enum TYPE_INDICATOR{
	NEW_LIVE_STREAMING_ONLY = 0,
	NEW_LIVE_STREAMING_GPS = NEW_LIVE_STREAMING_ONLY +1,//GPS also available
	NEW_RECORDED =NEW_LIVE_STREAMING_GPS +1 ,
	NEW_RECORDED_GPS = NEW_RECORDED +1,
	FILE_UPLOADED = NEW_RECORDED_GPS + 1 ,
	UPDATE_FILE_INFO = FILE_UPLOADED +1,
	NOT_SPECIFIED = UPDATE_FILE_INFO +1
};

class SegmentInformation {
public:
	SegmentInformation();
	virtual ~SegmentInformation();
	void addToList(StreamInformation* newStream);
	StreamInformation* getByOutputType(STREAM_OUTPUT_TYPE type);
	bool removeFromList(const std::string id);
	void setSegmentID(const std::string s);
	std::string getSegmentID();
	void setSourceType(STREAM_SOURCE_TYPE t);
	STREAM_SOURCE_TYPE getSourceType();
	StreamInformation* getByStreamID(const std::string id);
	void setIndicator(TYPE_INDICATOR i);
	TYPE_INDICATOR getIndicator();
	bool removeStreamByOutputType(STREAM_OUTPUT_TYPE type);
	void setGPSIncluded();
	bool isGPSIncluded();
	void setGPSAvailable();
	bool isGPSAvailable();
	StreamInformation* getFirstStream();
	void setVideoAvailable();
	bool isVideoAvailable();
	bool isVideoInfoReceived();
	bool isGPSInfoReceived();
	void setVideoInfoReceived();
	int isVideoDurationAvailable();
	void setVideoDurationAvailable();
	void setGPSInfoReceived();
	size_t getStreamCount();
	std::string getStoredFilePath(const std::string streamID);
	std::string getFirstStreamID();//streamID of the first entry in tempMap
	void addToTempList(std::string key, std::string value);
	void setStatus(PROCESSING_STATUS s);
	PROCESSING_STATUS getStatus();

private:
	std::string id;
	std::map<STREAM_OUTPUT_TYPE, StreamInformation*> streamMap;
	std::map<std::string, std::string> tempMap;//<segmentID, localFilePath>
	STREAM_SOURCE_TYPE sourceType;
	bool bIncludeGPS;
	bool bGPSUploaded;
	TYPE_INDICATOR indicator;
	bool bVideoUploaded;
	bool bGPSFileInfoReceived;
	bool bVideoFileInfoReceived;
	int bVideoDurationAvailable;
	PROCESSING_STATUS status;


};

#endif /* SEGMENTINFORMATION_H_ */
