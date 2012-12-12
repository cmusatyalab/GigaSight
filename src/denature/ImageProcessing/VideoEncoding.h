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
	VideoEncoding(SharedQueue<std::pair<int,cv::Mat> >* inputQueue, SharedQueue<SegmentInformation*>* s);
	virtual ~VideoEncoding();

	void operator()();
	void setOutputFilePath(STREAM_OUTPUT_TYPE type, std::string& path);
	bool configure();
	void stop();
	void loadConfiguration();
	void perfTest(int count);

private:
	SharedQueue<std::pair<int, cv::Mat> >* m_queue;
	std::string m_videoFilePath;
	std::string m_framesPath;
	SegmentInformation* segmentInfo;
	SharedQueue<SegmentInformation*> *segmentList;
	cv::VideoWriter* outputVideo;
	bool bEnding;
	std::string currentSegmentID;
	bool bWriteFrames;
	std::string host_;
	unsigned short port_;
	int sample_rate;
	int denature_sample_rate;
	int decoding_sample_rate;
};

#endif /* VIDEOENCODING_H_ */
