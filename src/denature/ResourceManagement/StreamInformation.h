/*
 * StreamInfomation.h
 *
 *  Created on: Oct 16, 2012
 *      Author: yuxiao
 */

#ifndef STREAMINFORMATION_H_
#define STREAMINFORMATION_H_
#include <string>
#include <list>
#include <opencv2/opencv.hpp>

enum PROCESSING_STATUS{
	INITIATED,
	WAITING_FOR_INPUT,
	PROCESSING,
	PAUSED,
	STOPPED
};

enum STREAM_SOURCE_TYPE{
	TYPE_NOT_SPECIFIED,
	TYPE_RECORDED,
	TYPE_LIVESTREAM
};

enum STREAM_OUTPUT_TYPE{
	TYPE_ORIGINAL_VIDEO,
	TYPE_DENATURED_VIDEO,
	TYPE_ORIGINAL_FRAME,
	TYPE_DENATURED_FRAME,
	TYPE_GPS_STREAM,
	TYPE_NO_OUTPUT
};

class StreamInformation {
public:
	StreamInformation();
	virtual ~StreamInformation();
	StreamInformation(const std::string segID, STREAM_OUTPUT_TYPE t2);
	void addTag(std::string& tag);
	void setStreamID(std::string& id);
	std::string& getStreamID();
	void setStatus(PROCESSING_STATUS s);
	PROCESSING_STATUS getStatus();
	//STREAM_SOURCE_TYPE getSourceType();
	STREAM_OUTPUT_TYPE getOutputType();
	std::string& getFilePath();
	void setVideoInfo(double fps, cv::Size size, int codec);
	double getVideoFrameRate();
	cv::Size getVideoFrameSize();
	int getInputVideoCodec();
	//void setSourceType(STREAM_SOURCE_TYPE t);
	void setOutputType(STREAM_OUTPUT_TYPE t);
	void setFilePath(std::string& s);

private:
	std::string stream_id;
	std::string segment_id;
	PROCESSING_STATUS status;
	//STREAM_SOURCE_TYPE inputType;
	STREAM_OUTPUT_TYPE outputType;
	int lastUpdatedCount;
	int currentCount;
	std::string sFSPath;//where to save the stream in NFS
	std::list<std::string> tags;
	cv::Size frameSize;
	double frameRate;
	int codecCode;
};

#endif /* STREAMINFORMATION_H_ */
