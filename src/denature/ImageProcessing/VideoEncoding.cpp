/*
 * VideoEncoding.cpp
 *
 *  Created on: Oct 4, 2012
 *      Author: yuxiao
 */

#include "VideoEncoding.h"
#include <sstream>

using namespace cv;

VideoEncoding::VideoEncoding() {
	// TODO Auto-generated constructor stub

}

VideoEncoding::~VideoEncoding() {
	// TODO Auto-generated destructor stub
}


VideoEncoding::VideoEncoding(SharedQueue<cv::Mat>* inputQueue, SegmentInformation* currentSegment){

	m_queue = inputQueue;
	segmentInfo = currentSegment;
	bEnding = false;
	bWriteFrames = false;
}


void VideoEncoding::operator()(){

	//get frames from the processedImageQueue and encode them into video files again
	int frameCount = 0;

	Mat frame;

	m_queue->wait_and_pop(frame);

	if (!configure())
		return;


	while (!bEnding) {

			//an empty frame is used to indicate the starting of a new stream
			if (frame.empty()){

				std::cout << frameCount << "frames encoded" << std::endl;

				if (!configure())
					break;

				frameCount = 0;

			}
			else {

				(*outputVideo) << frame;

				//save the denatured frames to FS. Actually decoding is quite light. Maybe it is unnecessary to save frames

				if (bWriteFrames){

					std::stringstream index;
					index << frameCount;

					imwrite(m_framesPath+"/"+index.str()+".jpeg", frame);
				}

				++frameCount;
			}

			m_queue->wait_and_pop(frame);

	}

	std::cout << "encoding stopped" << std::endl;

	if (outputVideo != NULL){
		outputVideo->release();
	}
}


void VideoEncoding::setOutputFilePath(STREAM_OUTPUT_TYPE type, std::string& path){

	if (type == TYPE_DENATURED_VIDEO)
		m_videoFilePath = path;
	else if (type == TYPE_DENATURED_FRAME)
		m_framesPath = path;
}


void VideoEncoding::stop(){
	bEnding = true;
}

bool VideoEncoding::configure(){

	/*Size frameSize;
	double fps = 30;
	int ex = 0;
	frameSize.height = 300;
	frameSize.width = 240;*/
	if (segmentInfo == NULL)
		return false;

	StreamInformation* denaturedVideoStream = segmentInfo->getByOutputType(TYPE_DENATURED_VIDEO);

	if (denaturedVideoStream != NULL){

		m_videoFilePath = denaturedVideoStream->getFilePath();

		if (m_videoFilePath.empty())
			return false;

		/* opens the file and initializes the video writer.
		* filename - the output file name.
		* fourcc - the codec
		* fps - the number of frames per second
		* frameSize - the video frame size
		* isColor - specifies whether the video stream is color or grayscale
		*/

		outputVideo = new VideoWriter(m_videoFilePath, CV_FOURCC('X', 'V', 'I', 'D'), denaturedVideoStream->getVideoFrameRate(), denaturedVideoStream->getVideoFrameSize(), true);

		//outputVideo.open(m_videoFilePath, denaturedVideoStream->getInputVideoCodec(), denaturedVideoStream->getVideoFrameRate(), denaturedVideoStream->getVideoFrameSize(), true);

		if ((outputVideo == NULL) || (!outputVideo->isOpened())){
				std::cout << "cannot initialize " << m_videoFilePath << std::endl;
				return false;
		}


	}
	else
		return false;

	StreamInformation* denaturedFrameStream = segmentInfo->getByOutputType(TYPE_DENATURED_FRAME);
	if (denaturedFrameStream != NULL){
		bWriteFrames = true;
		m_framesPath = denaturedFrameStream->getFilePath();
	}
	else
		bWriteFrames = false;


	return true;
}
