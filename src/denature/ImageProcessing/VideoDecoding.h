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
#include <boost/filesystem.hpp>

class VideoDecoding {
private:
	bool bLiveStreaming;
	bool bSaveOriginalVideo;//it can be done in video receiver actually
	std::string inputPath;
	SharedQueue<std::pair<int, cv::Mat> > *m_queue;
	bool bEnding, bPaused;
	SegmentInformation* currentSegment;
	SharedQueue<SegmentInformation*> *s_queue;//read segment information from this queue
	SharedQueue<SegmentInformation*> *s_output_queue;//forward segment information to imageprocessor
	SharedQueue<SegmentInformation*> *s_encode_queue;//forward segment information to encoder
	SharedQueue<std::pair<std::string, std::string> >* e_queue;
	cv::VideoCapture capture;
	//cv::VideoWriter* outputVideo;
	//std::string& currentVideoStreamID;
	int frameCount;
	std::map <std::string, SegmentInformation*> waitingList;
	std::string host_;//data manager
	unsigned int port_;
	bool bVideoCopied;
	bool bGPSCopied;
	int sample_rate;
	bool bEndTest;

	void notifyImageProcessor();
	void addToWaitingList(SegmentInformation* s);
	bool loadConfiguration();
	int updateSegmentInformation();

	int try_to_start_decoding();
	void copyFiles();
	std::string getNextFilePath();
	int fileCount;
	int index;
	std::string inputFolder;
	int frameNoLimit;

public:
	VideoDecoding();
	VideoDecoding(SharedQueue<std::pair<int, cv::Mat> >* queue, SharedQueue<SegmentInformation*>* s, SharedQueue<SegmentInformation*>* t, SharedQueue<SegmentInformation*> *q, SharedQueue<std::pair<std::string, std::string> >*v);
	virtual ~VideoDecoding();

	void operator()();//start decoding
	void stopDecoding();//when exit the app
	bool configure(SegmentInformation* s);
	void pauseDecoding();
	void resumeDecoding();
	void setInputPath(const std::string& path);
	bool getTestFilePath();
	void decodeFile(const boost::filesystem3::path& path, const int count);
	void setLimit(int i);
	void enableE2ETest();

};

#endif /* VIDEODECODING_H_ */
