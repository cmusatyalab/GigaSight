/*
 * VideoDecoding.h
 *
 *  Created on: Oct 4, 2012
 *      Author: yuxiao
 */

#ifndef VIDEODECODING_H_
#define VIDEODECODING_H_
#include <string>
#include <opencv2/opencv.hpp>
#include "SharedQueue.h"
#include "../ResourceManagement/SegmentInformation.h"


class VideoDecoding {
private:
	bool bLiveStreaming;
	bool bSaveOriginalVideo;//it can be done in video receiver actually
	std::string inputPath;
	SharedQueue<cv::Mat> *m_queue;
	bool bEnding, bPaused;
	SegmentInformation* currentSegment;
	cv::VideoCapture capture;
	cv::VideoWriter* outputVideo;
	//std::string& currentVideoStreamID;
	int frameCount;

public:
	VideoDecoding();
	VideoDecoding(SharedQueue<cv::Mat>* queue, SegmentInformation* s);

	virtual ~VideoDecoding();

	void operator()();//start decoding
	void stopDecoding();//when exit the app
	bool configure();
	void pauseDecoding();
	void resumeDecoding();
	void setInputPath(const std::string& path);
};

#endif /* VIDEODECODING_H_ */
