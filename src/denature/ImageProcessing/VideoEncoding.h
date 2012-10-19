/*
 * VideoEncoding.h
 *
 *  Created on: Oct 4, 2012
 *      Author: yuxiao
 *      Encode the processed frames into video again.
 */

#ifndef VIDEOENCODING_H_
#define VIDEOENCODING_H_
#include "SharedQueue.h"
#include <opencv2/opencv.hpp>
#include "../ResourceManagement/SegmentInformation.h"

class VideoEncoding {
public:
	VideoEncoding();
	VideoEncoding(SharedQueue<cv::Mat>* inputQueue, SegmentInformation* currentSegment);
	virtual ~VideoEncoding();

	void operator()();
	void setOutputFilePath(STREAM_OUTPUT_TYPE type, std::string& path);
	bool configure();
	void stop();

private:
	SharedQueue<cv::Mat>* m_queue;
	std::string m_videoFilePath;
	std::string m_framesPath;
	SegmentInformation* segmentInfo;
	cv::VideoWriter* outputVideo;
	bool bEnding;
	std::string currentSegmentID;
	bool bWriteFrames;
};

#endif /* VIDEOENCODING_H_ */
