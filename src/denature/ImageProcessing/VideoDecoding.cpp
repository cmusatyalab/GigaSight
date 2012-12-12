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
#include <jansson.h>
#include "../HTTPServer/Client.h"
#include <jansson.h>
#include <iostream>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include "AESEncryption.h"
#include <boost/thread.hpp>
#include <iostream>
#include <fstream>

using namespace cv;

VideoDecoding::VideoDecoding(){
	m_queue = NULL;
}

VideoDecoding::VideoDecoding(SharedQueue<std::pair<int,cv::Mat> > * queue,  SharedQueue<SegmentInformation*>* s, SharedQueue<SegmentInformation*>* t, SharedQueue<SegmentInformation*> *q, SharedQueue<std::pair<std::string, std::string> >*v) {
	// TODO Auto-generated constructor stub

	m_queue = queue;

	e_queue = v;

	bEnding = false;
	bLiveStreaming = false;
	bSaveOriginalVideo = false;//save the original video to file or not
	bPaused = false;
	//outputVideo = NULL;
	frameCount = 0;

	s_queue = s;
	s_output_queue = t;
	s_encode_queue = q;

	bVideoCopied = false;
	bGPSCopied = false;
	currentSegment = new SegmentInformation();

	inputPath ="";

	bEndTest = false;

	index = 0;

	frameNoLimit = -1;

}

VideoDecoding::~VideoDecoding() {
	// TODO Auto-generated destructor stub
	delete currentSegment;
	host_.clear();
	inputPath.clear();

	/*if (capture.isOpened())
		capture.release();*/
}

void VideoDecoding::copyFiles(){

	std::cout << "COPY FILE" << std::endl;

	StreamInformation* currentStream = NULL;

	if (currentSegment == NULL)
		return;

	if ((currentSegment->isVideoAvailable() && currentSegment->isVideoInfoReceived() && (!bVideoCopied)))
	{

		//write original video to NFS
		currentStream = currentSegment->getByOutputType(TYPE_ORIGINAL_VIDEO);

		boost::filesystem::path src(currentStream->getLocalFilePath());

		boost::filesystem::path dst(currentSegment->getByOutputType(TYPE_ORIGINAL_VIDEO)->getFilePath());

		if ((boost::filesystem::exists(dst)) && (boost::filesystem::exists(src))){
			//encrypt source file
			/*AESEncryption processor;

			unsigned char key_data[32];
			strcpy((char*)key_data,  "PI=3.1415926...");


			processor.aes_encrypt_file(src, key_data, dst);
			//boost::thread encryptionThread(boost::bind(&AESEncryption::aes_encrypt_file, &processor, src, key_data, dst));

			//boost::filesystem::copy_file(output, dst, boost::filesystem::copy_option::overwrite_if_exists);

			//boost::filesystem::copy_file(src, dst, boost::filesystem::copy_option::overwrite_if_exists);
			std::cout << "original video is copied from " << src.c_str() << " to " << dst.c_str() << std::endl;
			bVideoCopied = true;
			src.clear();
			dst.clear();*/

			if (e_queue != NULL)
				e_queue->push(std::pair<std::string, std::string>(currentStream->getLocalFilePath(), currentSegment->getByOutputType(TYPE_ORIGINAL_VIDEO)->getFilePath()));

		}
		else {
			std::cout << "cannot copy original video." << src.string() << " to " << dst.string() <<  std::endl;

			/*boost::filesystem3::path tempOutput("temp_encrypted");
			AESEncryption processor;

			unsigned char key_data[32];
			strcpy((char*)key_data,  "PI=3.1415926...");
			boost::thread encryptionThread(boost::bind(&AESEncryption::aes_encrypt_file, &processor, src, key_data, tempOutput));
			tempOutput.clear();
			src.clear();
			dst.clear();*/

			if (e_queue != NULL)
					e_queue->push(std::pair<std::string, std::string>(currentStream->getLocalFilePath(), "temp_encrypted"));
		}

	}


	if (currentSegment->isGPSAvailable() && currentSegment->isGPSInfoReceived() && (!bGPSCopied)){

		currentStream = currentSegment->getByOutputType(TYPE_GPS_STREAM);
		//write GPS stream to NFS
		boost::filesystem::path src(currentStream->getLocalFilePath());

		boost::filesystem::path dst(currentSegment->getByOutputType(TYPE_GPS_STREAM)->getFilePath());

		if ((boost::filesystem::exists(dst)) && (boost::filesystem::exists(src))){
			boost::filesystem::copy_file(src, dst, boost::filesystem::copy_option::overwrite_if_exists);
			bGPSCopied = true;
			std::cout << "GPS stream is copied from " << src.c_str() << " to " << dst.c_str() << std::endl;
		}

		src.clear();
		dst.clear();

	}


}


//update the state machine
//NEW_ will always arrive first. After that, FILE_UPLOADED and UPDATE_FILE_INFO may arrive in any random order.
//e.g. when video and gps streams are both included in the segment, only when NEW_, two FILE_UPLOADED and one UPDATE_FILE_INFO arrive, decoding will start
int VideoDecoding::updateSegmentInformation(){

	std::cout << "update segment info" << std::endl;

	std::map<std::string, SegmentInformation*>::iterator it;


	if (currentSegment == NULL)
		return -1;

	TYPE_INDICATOR indicator = currentSegment->getIndicator();

	switch (indicator){

		case NEW_LIVE_STREAMING_ONLY:{

			if (!configure(currentSegment)){
					std::cout << "failed to configure decoder" << std::endl;
					return -1;
			}

			return 1;
		}

		case NEW_LIVE_STREAMING_GPS:
		case NEW_RECORDED:
		case NEW_RECORDED_GPS:{
			std::cout << "NEW SEGMENT" << std::endl;

			//NEW_ is supposed to arrive earlier than UPDATE_FILE_INFO and FILE_UPLOADED
			if (waitingList.find(currentSegment->getSegmentID()) == waitingList.end()){
				addToWaitingList(currentSegment);
				return 0;
			}

			break;
		}

		case UPDATE_FILE_INFO:{
			std::cout << "UPDATE_FILE_INFO" << std::endl;

			it = waitingList.find(currentSegment->getSegmentID());

			if (it == waitingList.end()){
				addToWaitingList(currentSegment);
				return 0;
			}

			StreamInformation* oldRecord;
			StreamInformation* newRecord;

			STREAM_OUTPUT_TYPE type = TYPE_ORIGINAL_VIDEO;

			while (type != TYPE_UNKNOWN){

				oldRecord = (*it).second->getByOutputType(type);
				newRecord = currentSegment->getByOutputType(type);

				if (newRecord != NULL){

					if (oldRecord != NULL){
						oldRecord->setDuration(newRecord->getDuration());
						oldRecord->setStartTime(newRecord->getStartTime());

						if (type == TYPE_DENATURED_VIDEO){
							(*it).second->setVideoDurationAvailable();
							std::cout << "Denatured Video Info received" << std::endl;
						}else if (type == TYPE_ORIGINAL_VIDEO){
							if (bEndTest){
								std::string temp = getNextFilePath();
								oldRecord->setLocalFilePath(temp);
								std::cout << "set local file path to " << temp << std::endl;
								(*it).second->setVideoAvailable();
							}
						}

						if (type == TYPE_GPS_STREAM){
							(*it).second->setGPSInfoReceived();
							std::cout << "GPS Info received";
						}


					}
					else {
						//this one happens only when NEW_ arrives later, but it should not be so
						std::cout << "RARE" << std::endl;
						(*it).second->addToList(newRecord);
					}
				}


				if (type == TYPE_ORIGINAL_VIDEO)
					type = TYPE_DENATURED_VIDEO;
				else if (type == TYPE_DENATURED_VIDEO)
					type = TYPE_GPS_STREAM;
				else if (type == TYPE_GPS_STREAM)
					type = TYPE_UNKNOWN;

			} //while

			break;
		}

		case FILE_UPLOADED:{

			std::cout << "FILE UPLOADED" << std::endl;

			it = waitingList.find(currentSegment->getSegmentID());

			if (it == waitingList.end()){
				std::cout << "SegmentID" << currentSegment->getSegmentID() << " cannot be found from the list " << std::endl;
				return 0;
			}


			STREAM_OUTPUT_TYPE type = TYPE_ORIGINAL_VIDEO;
			StreamInformation* currentStream;
			std::string localFilePath;

			while (type != TYPE_UNKNOWN ){

					currentStream = (*it).second->getByOutputType(type);

					if (currentStream != NULL){

						localFilePath = currentSegment->getStoredFilePath(currentStream->getStreamID());

						if (!localFilePath.empty()){
							currentStream->setLocalFilePath(localFilePath);

							if (type == TYPE_ORIGINAL_VIDEO)
								(*it).second->setVideoAvailable();
							else
								(*it).second->setGPSAvailable();

							break;

						}
					}// if currentStream != NULL

					if (type == TYPE_ORIGINAL_VIDEO)
							type = TYPE_GPS_STREAM;
					else if (type == TYPE_GPS_STREAM)
						type = TYPE_UNKNOWN;
			}//while

			break;
		}//case

		default:{
			std::cout << "default" << std::endl;
			return -1;
		}
 	 }//switch

	//check if all the required information is available already

	currentSegment = (*it).second;

	int ret = try_to_start_decoding();

	if (ret != 0){
		waitingList.erase(it);
	}

	return ret;

}


//return 1, if all info has arrived
int VideoDecoding::try_to_start_decoding(){

	if (currentSegment == NULL)
		return -1;

	if (currentSegment->isGPSIncluded()){

		if ((!currentSegment->isGPSAvailable()) ||(!currentSegment->isGPSInfoReceived())){
				std::cout << "still waiting for GPS" << std::endl;
				return 0;
		}

	}else
		std::cout << "GPS is not required" << std::endl;


	if ((!currentSegment->isVideoAvailable()) || (!currentSegment->isVideoInfoReceived()) || (0 == currentSegment->isVideoDurationAvailable())) {
		std::cout << "videoinfo or video file has not arrived yet" << std::endl;
		return 0;
	}


	//copyFiles();


	if (!configure(currentSegment)){
		std::cout << "configuration failed" << std::endl;
		return -1;
	}


	return 1;

}

//when the thread starts, it will invoke operator()()
void VideoDecoding::operator()(){

	std::ofstream logFile;

	boost::posix_time::ptime mst1, mst2;
	boost::posix_time::time_duration msdiff;

	if (!loadConfiguration()){
		std::cout << "failed to configure video decoder" << std::endl;
		return;
	}

	//std::cout << "waiting for new segment to arrive" << std::endl;

	Mat rawFrame;

	if (s_queue == NULL)
		return;

	//for testing
	//rawFrame = imread("TestFiles/mvc-008f.jpg", CV_LOAD_IMAGE_COLOR);
	logFile.open("Log/decoding", std::ios::out | std::ios::app);

	if (!logFile.is_open())
		return;

	while (!bEnding) {

		if (s_queue != NULL)
			s_queue->wait_and_pop(currentSegment);

		mst1 = boost::posix_time::microsec_clock::local_time();

		while (bPaused){
			sleep(0.1);
		}

		if (updateSegmentInformation() < 1){
			continue;
		} else {

			bGPSCopied = false;
			bVideoCopied = false;

		}

		frameCount = 0;

		//std::cout << "start decoding" << std::endl;

		//read frames from the input file/stream
		capture >> rawFrame;


		while (!rawFrame.empty()) {


			++frameCount;

			//push back to the rawImageQueue, from frame1 to frame N
			//m_queue->push(std::pair<int, cv::Mat>(frameCount, rawFrame.clone()));


			if (m_queue != NULL){

				if (frameCount%sample_rate == 0){
					m_queue->push(std::pair<int, Mat>(frameCount, rawFrame.clone()));
					std::cout << "decode " << frameCount << std::endl;
				}


				//in case the image processor is much slower than decoding
				while (m_queue->size() > 50){
					sleep(0.5);
				}


			}

			//std::cout << "before release" << std::endl;
			rawFrame.release();//for test

			if (frameCount == frameNoLimit)
				break;

			capture >> rawFrame;

		}

		//std::cout << "Segment with ID: " << currentSegment->getSegmentID()<< " has been decoded "<<std::endl;


		notifyImageProcessor();

		if (capture.isOpened())
			capture.release();


		mst2 = boost::posix_time::microsec_clock::local_time();

		msdiff = mst2 - mst1;

		logFile << msdiff.total_milliseconds() << ";" << frameCount << ";" << "\r\n";


		std::cout << frameCount << " frames have been decoded." << std::endl;

		copyFiles();

	}
	

	logFile.close();

	if (capture.isOpened())
		capture.release();
	//capture.release();

	/*if (outputVideo != NULL){
		outputVideo->release();
	}*/

	std::cout << "VideoDecoding stops" << std::endl;
}


//decode a file or all the files in a folder
void VideoDecoding::decodeFile(const boost::filesystem3::path& path, const int count){

	loadConfiguration();

	boost::filesystem3::directory_iterator end_iter;

	if (!boost::filesystem3::exists(path)){
		return;
	}

	if (boost::filesystem3::is_directory(path)){

		for( boost::filesystem3::directory_iterator dir_iter(path) ; dir_iter != end_iter ; ++dir_iter)
		{
			decodeFile((*dir_iter).path(), count);

		}

	}else{

		Mat rawFrame;
		int frameCount = 0;
		std::ofstream logFile;
		logFile.open("Log/decoding", std::ios::out | std::ios::app);

		VideoCapture cap;

		boost::posix_time::ptime mst1;
		mst1 = boost::posix_time::microsec_clock::local_time();

		cap.open(path.c_str());

		if (!cap.isOpened()){
			std::cout << "cannot open video file" << std::endl;
			return;
		}



		//std::cout << "start decoding " << std::endl;

		cap >> rawFrame;


		while (!rawFrame.empty()) {


			++frameCount;

			//std::cout << "frameCount" << frameCount << std::endl;

			cap >> rawFrame;


			if (m_queue != NULL){

				if (frameCount%sample_rate == 0)
					m_queue->push(std::pair<int, Mat>(frameCount, rawFrame.clone()));

				//std::cout << "m_queue size is " << m_queue->size() << std::endl;

				while (m_queue->size() > 50){
					sleep(0.5);
				}

			}

			/*std::stringstream index;
			index << frameCount;
			imwrite("images/"+index.str()+".jpg", rawFrame);
			index.clear();*/


			if ((count != -1) && (frameCount == count)){
				Mat emptyMat;
				for (int i=0; i< 8; i++){
					if (m_queue != NULL)
						m_queue->push(std::pair<int, Mat>(-1, emptyMat));
				}

				break;
			}

		}

		boost::posix_time::ptime mst2 = boost::posix_time::microsec_clock::local_time();
		boost::posix_time::time_duration msdiff = mst2 - mst1;



		logFile << path.filename().c_str() << ";";
		logFile << frameCount << ";" ;
		logFile << (int)cap.get(CV_CAP_PROP_FPS) << ";";
		logFile << (int) cap.get(CV_CAP_PROP_FRAME_WIDTH) << ";" << (int) cap.get(CV_CAP_PROP_FRAME_HEIGHT) << ";";
		logFile << msdiff.total_milliseconds() << ";" << frameCount*1000/msdiff.total_milliseconds() << ";" << std::endl;

		frameCount = 0;

		cap.release();

		logFile.close();


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
bool VideoDecoding::configure(SegmentInformation* s){

	if (s == NULL){
		std::cout << "configure segment is empty" << std::endl;
		return false;
	}

	StreamInformation * rawVideo;

	std::cout << "videodecoding::configure" << std::endl;

	rawVideo = s->getByOutputType(TYPE_ORIGINAL_VIDEO);

	if (rawVideo == NULL){
		rawVideo = s->getByOutputType(TYPE_DENATURED_VIDEO);
	}


	if (rawVideo != NULL){
			inputPath = rawVideo->getLocalFilePath();
		}
	else{
			std::cout << "rawVideo is empty" << std::endl;
			return false;
	}



	if (inputPath.empty()){
		std::cout << "input file path is missing" << std::endl;
		return false;
	}


	if (capture.isOpened()){
		std::cout << "capture is opened. should be closed first" << std::endl;
		capture.release();
	}

	if (!capture.open(inputPath)){
			std::cout << "cannot open " << inputPath << std::endl;
			return false;
	}


	//get video propertiesSharedQueue
	Size frameSize = Size((int) capture.get(CV_CAP_PROP_FRAME_WIDTH), (int) capture.get(CV_CAP_PROP_FRAME_HEIGHT));
	double fps = capture.get(CV_CAP_PROP_FPS);

	//check codec
	int ex = static_cast<int>(capture.get(CV_CAP_PROP_FOURCC));//get codec
	char EXT[] = {ex & 0XFF , (ex & 0XFF00) >> 8,(ex & 0XFF0000) >> 16,(ex & 0XFF000000) >> 24, 0};

	int count = capture.get(CV_CAP_PROP_FRAME_COUNT);
	std::cout << "fps:" << fps <<std::endl;
	std::cout << "frame count: " << count << std::endl;
	std::cout << "frame width: "<< frameSize.width << std::endl;
	std::cout << "frame height: " << frameSize.height << std::endl;
	std::cout << "codec: " << ex << std::endl;
	std::cout << "EXT" << EXT << std::endl;


	StreamInformation* originalVideo = NULL;
	StreamInformation* denaturedVideo = NULL;

	if (s != NULL){

		originalVideo = s->getByOutputType(TYPE_ORIGINAL_VIDEO);
		denaturedVideo = s->getByOutputType(TYPE_DENATURED_VIDEO);

	}


	if (originalVideo != NULL){

		std::cout << "real duration " << originalVideo->getDuration() << std::endl;
		if (originalVideo->getDuration() > 0){
			fps = capture.get(CV_CAP_PROP_FRAME_COUNT)/originalVideo->getDuration();
		}

		originalVideo->setVideoInfo(fps, frameSize, ex);

		if ((count < frameNoLimit) || (frameNoLimit == -1))
			originalVideo->setFrameCount(count/sample_rate);
		else{
			originalVideo->setFrameCount(frameNoLimit/sample_rate);
			std::cout << "frameNoLimit " << frameNoLimit << "sample_rate" << sample_rate << std::endl;
		}



		bSaveOriginalVideo = true;

		//send PUT to data manager
		Client c;
		json_t* request = json_object();
		json_object_set_new(request, "frame_rate", json_real(fps));
		json_object_set_new(request, "frame_width", json_real(frameSize.width));
		json_object_set_new(request, "frame_height", json_real(frameSize.height));
		json_object_set_new(request, "frame_count", json_real(capture.get(CV_CAP_PROP_FRAME_COUNT)));

		c.updateSegmentInformation(host_, port_, s->getSegmentID(), json_dumps(request, 0));
	}
	else
		bSaveOriginalVideo = false;



	if (denaturedVideo != NULL)
		denaturedVideo->setVideoInfo(fps, frameSize, ex);


	//save original frames to a video file
	/*if (bSaveOriginalVideo){


			string outputVideoFileName = originalVideo->getFilePath();

			if (outputVideoFileName.empty()){
				std::cout << "Output file path for original video is empty" << std::endl;
				return false;
			}

			boost::filesystem3::path path(outputVideoFileName);
			if (!boost::filesystem3::exists(path)){

				outputVideoFileName = "original.avi";
				std::cout << outputVideoFileName << "for original video has not been created" << std::endl;

			}

			// opens the file and initializes the video writer.
		    // filename - the output file name.
		    // fourcc - the codec
		    // fps - the number of frames per second
		    // frameSize - the video frame size
		    // isColor - specifies whether the video stream is color or grayscale

			//currently, we can not save it to .mp4 file, but .avi file is fine
			outputVideo = new VideoWriter(outputVideoFileName, CV_FOURCC('X', 'V', 'I', 'D'), fps, frameSize, true);

			//outputVideo = new VideoWriter(outputVideoFileName, CV_FOURCC('D', 'I', 'V', 'X'), fps, frameSize, true);

		    if ((outputVideo == NULL) || (!outputVideo->isOpened())){
		    	std::cout << "cannot initialize " << outputVideoFileName << std::endl;
		    	bEnding = true;
		    	return false;
		    }

		    std::cout << "Original video is saved to " << outputVideoFileName << std::endl;
	}

*/

	frameCount = 0;

	std::cout << "decoder configuration finished" << std::endl;

	s_output_queue->push(s);
	std::cout << "s_output_queue size" << s_output_queue->size() << std::endl;

	s_encode_queue->push(s);
	std::cout << "s_encode_queue size" << s_encode_queue->size() << std::endl;


	return true;
}


void VideoDecoding::setInputPath(const std::string& path){

	inputPath = path;
}



bool VideoDecoding::getTestFilePath(){

	json_error_t err_t;
	json_t* root = json_load_file("XMLFiles/config.json", 0, &err_t);
	json_t* path = NULL;

	if (root != NULL){
		path = json_object_get(root, "INPUT_VIDEO_PATH");
	}

	if (path != NULL){
		inputPath = json_string_value(path);
	}

	std::cout << "Test file:" << inputPath << std::endl;
	json_object_clear(path);
	json_object_clear(root);

	return (!inputPath.empty());

}

void VideoDecoding::notifyImageProcessor(){
	//push an empty frame when it is reset
	Mat emptyFrame;

	if (emptyFrame.empty()){

		std::cout << "push an empty frame to rawFrameQueue" << std::endl;
		m_queue->push(std::pair<int, cv::Mat>(0, emptyFrame));

	}
}


void VideoDecoding::addToWaitingList(SegmentInformation* s){

	waitingList.insert(std::pair<std::string, SegmentInformation*>(currentSegment->getSegmentID(), s));

	std::cout << "add to waiting list " << currentSegment->getSegmentID() << std::endl;

}



bool VideoDecoding::loadConfiguration(){

	json_error_t err_t;
	json_t* root = json_load_file("XMLFiles/config.json", 0, &err_t);
	json_t* dmAddr = NULL;
	json_t* dmPort = NULL;
	json_t* rate = NULL;
	json_t* path = NULL;

	if (root != NULL){

		dmAddr = json_object_get(root, "DATAMANAGER_URI");

		dmPort = json_object_get(root, "DATAMANAGER_PORT");

		rate = json_object_get(root, "DECODING_SAMPLE_RATE");

		path = json_object_get(root, "TEST_FILES");
	}

	if (dmAddr != NULL)
		host_ = json_string_value(dmAddr);
	else
		host_ = "127.0.0.1";

	if (dmPort != NULL)
		port_ = atoi(json_string_value(dmPort));
	else
		port_ = 5000;

	if (rate != NULL)
		sample_rate = json_integer_value(rate);
	else
		sample_rate = 1;

	if ((path != NULL) && (bEndTest)){
		inputFolder = json_string_value(path);

		boost::filesystem3::path filePath(inputFolder);

		if (!boost::filesystem3::exists(filePath)){
				filePath.clear();
				return false;
		}

		//currently, we only support one layer of folder
		if (boost::filesystem3::is_directory(filePath)){

			fileCount = 0;
			boost::filesystem3::directory_iterator end_iter;
			for( boost::filesystem3::directory_iterator dir_iter(filePath) ; dir_iter != end_iter ; ++dir_iter)
			{
				if (boost::filesystem3::is_regular_file((*dir_iter).path()))
					fileCount++;
			}
		}
		else
			fileCount = 1;

		filePath.clear();
	}

	json_object_clear(dmAddr);
	json_object_clear(dmPort);
	json_object_clear(rate);
	json_object_clear(path);
	json_object_clear(root);

	std::cout << "Decoder configuration" << std::endl;

	return true;

}

std::string VideoDecoding::getNextFilePath(){

	boost::filesystem3::directory_iterator end_iter;

	boost::filesystem3::path path(inputFolder);

	if (boost::filesystem3::exists(path)){

		if (boost::filesystem3::is_regular_file(path))
			return path.string();

		boost::filesystem3::directory_iterator dir_iter(path);

		if (index < fileCount){
			for (int j=0; j<index; j++)
				++dir_iter;

			index++;

			return (*dir_iter).path().string();
		}
		else
			return "";

	}else
		return "";

}


void VideoDecoding::setLimit(int i){

	frameNoLimit = i;
}


void VideoDecoding::enableE2ETest(){
	bEndTest = true;
}
