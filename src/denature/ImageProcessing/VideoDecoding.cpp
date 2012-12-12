/*
 * VideoDecoding.cpp
 *
 *  Created on: Oct 4, 2012
 *      Author: yuxiao
 *      This class is responsible for decoding the incoming video streams.
 *      The video stream is parsed into frames.
 *      Frames are added to the end of a queue, waiting for other threads to process them.
 */

#include "VideoDecoding.h"
#include <opencv2/core/core.hpp>
//#include <opencv2/highgui/highgui.hpp>


using namespace cv;

VideoDecoding::VideoDecoding(){
}


VideoDecoding::VideoDecoding(SharedQueue<cv::Mat> * queue,  SegmentInformation* s) {
	// TODO Auto-generated constructor stub

	m_queue = queue;

	bEnding = false;
	bLiveStreaming = false;
	bSaveOriginalVideo = false;//save the original video to file or not
	bPaused = false;
	outputVideo = NULL;
	frameCount = 0;

	currentSegment = s;
}

VideoDecoding::~VideoDecoding() {
	// TODO Auto-generated destructor stub
}

//when the thread starts, it will invoke operator()()
void VideoDecoding::operator()(){

	//use testFace.mp4 for testing only
	//setInputPath("TestFiles/testFace.mp4");

	//e.g. listening on port 5555
	setInputPath("udp://128.237.233.144:8080");

	//wait for signal to start. can be improved later
	while (!configure()){

		sleep(1);
	}
	
	Mat rawFrame;

	//for testing
	//rawFrame = imread("TestFiles/mvc-008f.jpg", CV_LOAD_IMAGE_COLOR);

	while (!bEnding) {

		while (bPaused){
			sleep(0.1);
		}


		//read frames from the input file/stream
		capture >> rawFrame;


		/* use some images for testing
		 *
		if (frameCount == 0)
			rawFrame = imread("mvc-008f.jpg", CV_LOAD_IMAGE_COLOR);

		if (frameCount == 1)
			rawFrame = imread("smart-car.jpg", CV_LOAD_IMAGE_COLOR);

		if (frameCount == 2)
			rawFrame = imread("mvc-009f.jpg", CV_LOAD_IMAGE_COLOR);
		*/

		if (rawFrame.empty()) {
				std::cout << "This stream has finished "<<std::endl;
				std::cout << frameCount << " frames have been read." << std::endl;


				if (bLiveStreaming){
					sleep(1);
					continue;
				}else
					break;
		}
	

		++frameCount;
		
		//push back to the rawImageQueue
		m_queue->push(rawFrame.clone());
		
		std::cout << "pushing" << m_queue->size() << std::endl;

		if (bSaveOriginalVideo){
			(*outputVideo) << rawFrame;
		}

		//for testing begin
		if (frameCount > 450)
			bEnding = true;
		//for testing end

		//rawFrame = imread("TestFiles/mvc-009f.jpg", CV_LOAD_IMAGE_COLOR);
	}
	


	capture.release();

	if (outputVideo != NULL){
		outputVideo->release();
	}
}


void VideoDecoding::stopDecoding(){

	bEnding = true;

}

void VideoDecoding::pauseDecoding(){
	bPaused = true;
}

void VideoDecoding::resumeDecoding(){
	bPaused = false;
}


//this should be triggered after the streaming starts. some frames at the beginning will be lost
//in practice, first pauseDecoding(), then reset(), after that resumeDecoding()
bool VideoDecoding::configure(){

	//where to get inputPath??

	if (inputPath.empty()){
		//std::cout << "input file path is missing" << std::endl;
		return false;
	}


	if (outputVideo != NULL){
		outputVideo->release();
		outputVideo = NULL;
	}


	if (!capture.open(inputPath)){
			std::cout << "cannot open " << inputPath << std::endl;
			bEnding = true;
			return false;
	}


	//get video properties
	Size frameSize = Size((int) capture.get(CV_CAP_PROP_FRAME_WIDTH), (int) capture.get(CV_CAP_PROP_FRAME_HEIGHT));
	double fps = capture.get(CV_CAP_PROP_FPS);

	//check codec
	int ex = static_cast<int>(capture.get(CV_CAP_PROP_FOURCC));//get codec
	char EXT[] = {ex & 0XFF , (ex & 0XFF00) >> 8,(ex & 0XFF0000) >> 16,(ex & 0XFF000000) >> 24, 0};

	std::cout << "fps:" << fps <<std::endl;
	std::cout << "frame count: " << capture.get(CV_CAP_PROP_FRAME_COUNT) << std::endl;
	std::cout << "frame width: "<< frameSize.width << std::endl;
	std::cout << "frame height: " << frameSize.height << std::endl;
	std::cout << "codec: " << ex << std::endl;
	std::cout << "EXT" << EXT << std::endl;

	StreamInformation* originalVideo = NULL;
	StreamInformation* denaturedVideo = NULL;

	if (currentSegment != NULL){

		/*
		StreamInformation* v = new StreamInformation();
		v->setVideoInfo(fps, frameSize, ex);
		v->setOutputType(TYPE_ORIGINAL_VIDEO);
		std::string y = "o.avi";
		v->setFilePath(y);
		currentSegment->addToList(v);

		StreamInformation* v1 = new StreamInformation();
		v1->setVideoInfo(fps, frameSize, ex);
		v1->setOutputType(TYPE_DENATURED_VIDEO);
		std::string x = "d.avi";
		v1->setFilePath(x);
		currentSegment->addToList(v1);*/

		originalVideo = currentSegment->getByOutputType(TYPE_ORIGINAL_VIDEO);
		denaturedVideo = currentSegment->getByOutputType(TYPE_DENATURED_VIDEO);

	}


	if (originalVideo != NULL){
		originalVideo->setVideoInfo(fps, frameSize, ex);
		bSaveOriginalVideo = true;
		//currentVideoStreamID = originalVideo->getStreamID();

	}
	else
		bSaveOriginalVideo = false;



	if (denaturedVideo != NULL){
		denaturedVideo->setVideoInfo(fps, frameSize, ex);
		std::cout << "test 12" << std::endl;
	}

	//save original frames to a video file
	if (bSaveOriginalVideo){

			const string outputVideoFileName = originalVideo->getFilePath();

			if (outputVideoFileName.empty())
				return false;

			// opens the file and initializes the video writer.
		    // filename - the output file name.
		    // fourcc - the codec
		    // fps - the number of frames per second
		    // frameSize - the video frame size
		    // isColor - specifies whether the video stream is color or grayscale

			//currently, we can not save it to .mp4 file, but .avi file is fine
			outputVideo = new VideoWriter(outputVideoFileName, CV_FOURCC('X', 'V', 'I', 'D'), fps, frameSize, true);

		    if ((outputVideo == NULL) || (!outputVideo->isOpened())){
		    	std::cout << "cannot initialize " << outputVideoFileName << std::endl;
		    	bEnding = true;
		    	return false;
		    }
	}



	frameCount = 0;

	//push an empty frame when it is reset
	Mat emptyFrame;
	if (emptyFrame.empty()){
		std::cout << "push an empty frame to rawFrameQueue" << std::endl;
		m_queue->push(emptyFrame);
	}

	std::cout << "decoder configuration finished" << std::endl;

	//test begins
	/*
	*bSaveOriginalVideo = true;

	string f = "rawVideo.avi";

	outputVideo = new VideoWriter(f, CV_FOURCC('X', 'V', 'I', 'D'), fps, frameSize, true);

	if ((outputVideo != NULL) && (!outputVideo->isOpened())){
	   	std::cout << "cannot initialize " << f << std::endl;
	  	bEnding = true;
	   	return false;
	}*/ //test ends

	return true;
}


void VideoDecoding::setInputPath(const std::string& path){

	inputPath = path;
}
