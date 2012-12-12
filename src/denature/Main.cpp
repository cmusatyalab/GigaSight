/*
 * Main.cpp
 *
 *  Created on: Oct 4, 2012
 *      Author: yuxiao
 */

#include <list>
#include <queue>
#include <boost/thread.hpp>
#include <opencv2/opencv.hpp>
#include "ResourceManagement/ProcessingRuleContainer.h"
#include "ImageProcessing/VideoDecoding.h"
#include "ImageProcessing/SharedQueue.h"
#include "ImageProcessing/ImageProcessing.h"
#include "ImageProcessing/VideoEncoding.h"
#include "HTTPServer/server.hpp"
#include "ResourceManagement/SegmentInformation.h"
#include "HTTPServer/Client.h"



int main(int argc, char *argv[]){

	//currently, the Datamanager IP& port is hardcoded in HTTPServer/request_handler.cpp. Later, we can load it from configuration file
	//for opencv::capture, the input file path (e.g. testFace.mp4) is now hardcoded.
	//the Http server should inform the VideoDecoding thread the input filepath (not added to the code yet. should be added to VideoDecoding::configure())

	//initialize two sharedqueue, one for raw frames, and the other for denatured frames
	SharedQueue<cv::Mat> *rawImageQueue = new SharedQueue<cv::Mat>();

	SharedQueue<cv::Mat> *processedImageQueue = new SharedQueue<cv::Mat>();
	
	ProcessingRuleContainer* ruleManager = new ProcessingRuleContainer();

	SegmentInformation* currentSegment = new SegmentInformation();


	//new a thread for video decoding
	VideoDecoding videoDecoder(rawImageQueue, currentSegment);

	boost::thread decodingThread(videoDecoder);
	
	//new a thread for image processing. actually, we can new several imageprocessing threads if necessary
	ImageProcessing videoProcessor(rawImageQueue, processedImageQueue, ruleManager);

	boost::thread processingThread(videoProcessor);

	//if there is only one processing thread, we can choose to include encoding in the ImageProcessing thread
	//then we don't need to use processedImageQueue, we can directly write the frames to file system

	VideoEncoding videoEncoder(processedImageQueue, currentSegment);

	boost::thread encodingThread(videoEncoder);


	http::server::server s("127.0.0.1","12348", currentSegment, ruleManager);

	s.run();

	//join() will block other threads until this thread finishes.Only for testing.
	decodingThread.join();
	processingThread.join();
	encodingThread.join();

	//pop up the images left in the queues
	rawImageQueue->clean();
	processedImageQueue->clean();

	delete ruleManager;
	delete currentSegment;


/*
	std::string result;
	Client c;
	c.requestForSegment("monsoon.elijah.cs.cmu.edu", 5000, "1", result);

	std::cout << "result" << result << std::endl;
*/

}
