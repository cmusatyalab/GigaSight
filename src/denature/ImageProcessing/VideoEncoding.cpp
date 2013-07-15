/*
 * VideoEncoding.cpp
 *
 *  Created on: Oct 4, 2012
 *      Author: yuxiao
 */

#include "VideoEncoding.h"
#include <sstream>
#include "../HTTPServer/Client.h"
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <iostream>
#include <fstream>


using namespace cv;

VideoEncoding::VideoEncoding() {
	// TODO Auto-generated constructor stub

}

VideoEncoding::~VideoEncoding() {
	// TODO Auto-generated destructor stub
}


VideoEncoding::VideoEncoding(SharedQueue<std::pair<int,cv::Mat> >* inputQueue, SharedQueue<SegmentInformation*>* s){

	m_queue = inputQueue;
	segmentInfo = NULL;
	segmentList = s;
	bEnding = false;
	bWriteFrames = false;

	loadConfiguration();
}


void VideoEncoding::operator()(){

	//get frames from the processedImageQueue and encode them into video files again
	int frameCount = -1;
	int prevIndex = -1;
	int BUFFER_SIZE = 100;
	std::pair<int, Mat> buffer[BUFFER_SIZE];
	std::pair<int, Mat> backupBuffer[BUFFER_SIZE];
	int bufferIndex = 0;
	//bool bBackup = false;
	bool bNewVideo = false;

	int newStart = -1;

	for (int i=0; i<BUFFER_SIZE; i++){
		buffer[i].first = -1;
		memset(buffer[i].second.data, 0, *buffer[i].second.size.p);
		backupBuffer[i].first = -1;
		memset(backupBuffer[i].second.data, 0, *backupBuffer[i].second.size.p);
	}

	//Mat frame;

	std::pair<int, cv::Mat> frame;
	StreamInformation* finishedSeg = NULL;

	std::ofstream logFile;
	boost::posix_time::ptime mst1, mst2;
	boost::posix_time::time_duration msdiff;

	std::string fileName;
	std::ostringstream x;
	x << (unsigned int)pthread_self();
	fileName.assign("Log/encoding_" + x.str());
	x.clear();

	//bool bDirty = false;
	int step = decoding_sample_rate;

	while (!bEnding) {

		m_queue->wait_and_pop(frame);

		bNewVideo = false;

		if (frame.first < prevIndex){

			std::cout << "new video" << std::endl;

			bNewVideo = true;

		}


		//std::cout << "frame encoding" << std::endl;
		//when the program runs for the first time
		if (prevIndex == -1){

			std::cout << "encoder received the first frame" << std::endl;

			mst1 = boost::posix_time::microsec_clock::local_time();

			logFile.open(fileName.c_str(), std::ios::out | std::ios::app);


			if (!configure()){
				std::cout << "encoder configuration failed" << std::endl;
				continue;
			}

			if (segmentInfo != NULL)
				finishedSeg = segmentInfo->getByOutputType(TYPE_ORIGINAL_VIDEO);

			if (finishedSeg != NULL){
				frameCount = finishedSeg->getFrameCount();
				std::cout << "this stream should include " << finishedSeg->getFrameCount() << "frames"  << std::endl;
			}else
				std::cout << "finishedSeg is empty" << std::endl;

			prevIndex = 0;
			bufferIndex = 0;

		} else if (frameCount == 0){

			if (segmentInfo != NULL){
				finishedSeg = segmentInfo->getByOutputType(TYPE_DENATURED_VIDEO);

				if (finishedSeg != NULL){
					Client c;
					c.updateProcessingStatus(host_, port_, finishedSeg->getStreamID(), STOPPED);
				}

			}

			if (outputVideo != NULL){
				outputVideo->release();
				outputVideo = NULL;
			}

			mst1 = boost::posix_time::microsec_clock::local_time();

			logFile.open(fileName.c_str(), std::ios::out | std::ios::app);


			if (!configure())
				continue;

			if (segmentInfo != NULL)
				finishedSeg = segmentInfo->getByOutputType(TYPE_ORIGINAL_VIDEO);

			if (finishedSeg != NULL)
				frameCount = finishedSeg->getFrameCount();
			else
				break;


			prevIndex = denature_sample_rate - sample_rate;
			bufferIndex = 0;

			std::cout << "frame count is" << frameCount << "new start is "  << newStart << std::endl;

			if (newStart != -1){
				bool bDirty = false;
				while (buffer[newStart].first == prevIndex + step){

					(*outputVideo) << buffer[newStart].second;

					prevIndex = buffer[newStart].first;

					buffer[newStart].first = -1;
					buffer[newStart].second.release();
					newStart = (newStart+1)%BUFFER_SIZE;
					bDirty = true;
					frameCount--;
				}

				if (bDirty){
					newStart++;
					bufferIndex = newStart;
				}else
					bufferIndex = (newStart - buffer[newStart].first/step + 2)%BUFFER_SIZE;

				newStart = -1;


			}

			bNewVideo = false;

		}


		//an empty frame is used to indicate the starting of a new stream
		if (!frame.second.empty()){

			std::cout << "received " << frame.first << std::endl;


				if (frame.first == prevIndex + step){


					(*outputVideo) << frame.second;

					frame.second.release();

					prevIndex = frame.first;
					std::cout << "write " << frame.first << std::endl;
					frameCount--;


					while (buffer[bufferIndex].first != -1){

						std::cout << "write from buffer " << buffer[bufferIndex].first << std::endl;

						(*outputVideo) << buffer[bufferIndex].second;
						prevIndex = buffer[bufferIndex].first;

						buffer[bufferIndex].second.release();
						buffer[bufferIndex].first = -1;
						bufferIndex++;
						bufferIndex = bufferIndex%BUFFER_SIZE;
						frameCount--;

						if (frameCount == 0){
							break;
						}

					}


					if (buffer[bufferIndex].first == -1){

						bufferIndex++;
						bufferIndex = bufferIndex%BUFFER_SIZE;

					}

				}
				else{
					//push to buffer
					int index;

					if (bNewVideo){
						index = (bufferIndex + frameCount + frame.first/step - 1)%BUFFER_SIZE;

						if (newStart == -1)
							newStart = index;
						else if (frame.first < buffer[newStart].first)
							newStart = index;

						std::cout << "new start is " << newStart << "index" << index << std::endl;
					}
					else
						index = (bufferIndex+frame.first/step-prevIndex/step-2)%BUFFER_SIZE;




					if (buffer[index].first != -1){

						frameCount--;

						while (bufferIndex != index){

							if (buffer[bufferIndex].first != -1){
								(*outputVideo) << buffer[bufferIndex].second;
								std::cout << "FORCE write " << buffer[bufferIndex].first << "from buffer" << bufferIndex << std::endl;
								prevIndex = buffer[bufferIndex].first;
							}

							frameCount--;
							buffer[bufferIndex].second.release();
							buffer[bufferIndex].first = -1;
							bufferIndex = (bufferIndex+1)%BUFFER_SIZE;

							if (frameCount == 0)
								break;
						}

						while (buffer[bufferIndex].first != -1){

							if (frameCount == 0)
								break;

							(*outputVideo) << buffer[bufferIndex].second;
							std::cout << "FORCE write " << buffer[bufferIndex].first << std::endl;
							prevIndex = buffer[bufferIndex].first;

							frameCount--;
							buffer[bufferIndex].second.release();
							buffer[bufferIndex].first = -1;
							bufferIndex = (bufferIndex+1)%BUFFER_SIZE;



						}

						if (frameCount > 0){
							std::cout << "!!!!!!!! still more frames are missing" << std::endl;
						}


						if (buffer[index].first == -1){

							buffer[index].first = frame.first;
							buffer[index].second = frame.second.clone();

							std::cout << "push to buffer" << index << ": " << frame.first << std::endl;

							frame.second.release();
						} else{

							std::cout << "encoding error ............Quit" << std::endl;
							return;
						}

					}
					else{

							buffer[index].first = frame.first;
							buffer[index].second = frame.second.clone();

							std::cout << "push to buffer" << index << ": " << frame.first << std::endl;

							frame.second.release();
					}


				}

				//bufferIndex++;
				//bufferIndex = bufferIndex%10;

				//save the denatured frames to FS. Actually decoding is quite light. Maybe it is unnecessary to save frames

				if (bWriteFrames) {

					std::stringstream index;
					index << frame.first;

					imwrite(m_framesPath+"/"+index.str()+".jpeg", frame.second);
				}



				if (frameCount%1 == 0){
					std::cout << frameCount << " frames not encoded yet" << std::endl;
				}

				if (frameCount == 0){

					mst2 = boost::posix_time::microsec_clock::local_time();
					msdiff = mst2-mst1;
					logFile << msdiff.total_milliseconds() << ";" << "\r\n";

					logFile.close();
				}

		}


	}//while



	if (outputVideo != NULL){
		outputVideo->release();
	}

	std::cout << "encoding stopped" << std::endl;
}



void VideoEncoding::perfTest(int count){

	//get frames from the processedImageQueue and encode them into video files again
	int frameCount = count/decoding_sample_rate;
	int prevIndex = -1;
	int BUFFER_SIZE = 50;
	std::pair<int, Mat> buffer[BUFFER_SIZE];
	int bufferIndex = 0;

	for (int i=0; i<BUFFER_SIZE; i++){
		buffer[i].first = -1;
		memset(buffer[i].second.data, 0, *buffer[i].second.size.p);
	}

	std::pair<int, cv::Mat> frame;

	std::ofstream logFile;
	logFile.open("Log/encoding", std::ios::out | std::ios::app);

	boost::posix_time::ptime mst1;
	boost::posix_time::ptime mst2;
	boost::posix_time::time_duration msdiff;

	//mst1 = boost::posix_time::microsec_clock::local_time();

	while (frameCount > 0) {

			m_queue->wait_and_pop(frame);

			mst1 = boost::posix_time::microsec_clock::local_time();

			//std::cout << "frame encoding" << std::endl;
			//when the program runs for the first time
			if (prevIndex == -1){

				std::cout << "encoder received the first frame" << std::endl;

				m_videoFilePath = "denaturedVideo";

				outputVideo = new VideoWriter(m_videoFilePath, CV_FOURCC('X', 'V', 'I', 'D'), 30, cv::Size(frame.second.cols, frame.second.rows), true);

				if (!outputVideo->isOpened()){
					std::cout << "output video cannot be opened" << std::endl;
					return;
				}

				prevIndex = 0;
				bufferIndex = 0;

			}

			int step = decoding_sample_rate;
			//an empty frame is used to indicate the starting of a new stream
			if (!frame.second.empty()){

				//std::cout << "received " << frame.first << std::endl;

				if (bWriteFrames) {

					std::stringstream index;
					index << frame.first;

					imwrite("Images/"+index.str()+".jpeg", frame.second);
				}


				if (frame.first == prevIndex+ step){

						//std::cout << "try to encode" << frame.second.data << std::endl;
						(*outputVideo) << frame.second;

						prevIndex = frame.first;
						//std::cout << "write " << frame.first << std::endl;

						frame.second.release();//for test

						//std::cout << "release frame.second" << frame.first << std::endl;
						frameCount--;

						while (buffer[bufferIndex].first != -1){

							//std::cout << "write from buffer " << buffer[bufferIndex].first << std::endl;

							(*outputVideo) << buffer[bufferIndex].second;

							prevIndex = buffer[bufferIndex].first;

							if (bWriteFrames) {

									std::stringstream index;
									index << buffer[bufferIndex].first;

									imwrite("Images/"+index.str()+".jpeg", buffer[bufferIndex].second);
							}


							buffer[bufferIndex].second.release();
							//std::cout << "release " << buffer[bufferIndex].first << std::endl;
							buffer[bufferIndex].first = -1;
							bufferIndex++;
							bufferIndex = bufferIndex%BUFFER_SIZE;
							frameCount--;

						}//while

						bufferIndex++;
						bufferIndex = bufferIndex%BUFFER_SIZE;


					}
					else{
						//std::cout << "try to buffer " << std::endl;
						//push to buffer
						int index = (bufferIndex+frame.first/step-prevIndex/step-2)%BUFFER_SIZE;

						while (buffer[index].first != -1){
							std::cout << "encoding buffer is full. Quit!!" << std::endl;
							//sleep(1);

							for (int j =0; j<BUFFER_SIZE; j++){

								if (buffer[j].first != -1){
									buffer[j].first = -1;
									buffer[j].second.release();
								}

							}

							mst2 = boost::posix_time::microsec_clock::local_time();
							msdiff = mst2 - mst1;


							logFile << prevIndex << ";" ;
							logFile << msdiff.total_milliseconds() << ";" << "\r\n";
							return;
						}

						buffer[index].first = frame.first;
						buffer[index].second = frame.second.clone();


						frame.second.release();

						//std::cout << "push to buffer" << index << ": " << frame.first << std::endl;
					}

					mst2 = boost::posix_time::microsec_clock::local_time();
					msdiff = mst2 - mst1;


					logFile << prevIndex << ";" ;
					logFile << msdiff.total_microseconds() << ";" << std::endl;


					//save the denatured frames to FS. Actually decoding is quite light. Maybe it is unnecessary to save frames

					if (frameCount%10 == 0){
						std::cout << frameCount << " frames not encoded yet" << std::endl;
					}

			}else
				std::cout << "frame.second is empty" << std::endl;


		}//while



		if (outputVideo != NULL){
			outputVideo->release();
		}

		std::cout << "encoding stopped" << std::endl;


		logFile.close();
}



void VideoEncoding::setOutputFilePath(STREAM_OUTPUT_TYPE type, std::string& path){

	if (type == TYPE_DENATURED_VIDEO)
		m_videoFilePath = path;
	//else if (type == TYPE_DENATURED_FRAME)
		//m_framesPath = path;
}


void VideoEncoding::stop(){
	bEnding = true;
}

bool VideoEncoding::configure(){


	if (segmentList->try_pop(segmentInfo)) {

		if (segmentInfo == NULL){

			std::cout << "VideoEncoding:: segmentInformation is empty" << std::endl;
			return false;
		}
	}
	else{

		std::cout << "no segmentInfo available in segmentList" << std::endl;
		return false;

	}


	StreamInformation* denaturedVideoStream = segmentInfo->getByOutputType(TYPE_DENATURED_VIDEO);

	if (denaturedVideoStream != NULL){

		m_videoFilePath = denaturedVideoStream->getFilePath();

		if (m_videoFilePath.empty()){
			std::cout << "m_videoFilePath is empty" << std::endl;
			return false;
		}

		boost::filesystem3::path path(m_videoFilePath);

		if (!boost::filesystem3::exists(path)){
			std::cout << "The path does not exist. We will save the file to test.mp4v" << std::endl;
			m_videoFilePath = "test";
		}

        if (symlink(m_videoFilePath.c_str(),
                    (m_videoFilePath + ".avi").c_str()))
        {
            fprintf(stderr, "--> FAILED CREATING SYMLINK\n");
        }

		/* opens the file and initializes the video writer.
		* filename - the output file name.
		* fourcc - the codec
		* fps - the number of frames per second
		* frameSize - the video frame size
		* isColor - specifies whether the video stream is color or grayscale
		*/


		// according to home/yuxiao/software/OpenCV-2.4.2/modules/highgui/src/cap_gstreamer.cpp
		//only the following one is supported
		/* encs[CV_FOURCC('H','F','Y','U')]=(char*)"ffenc_huffyuv";
    	*encs[CV_FOURCC('D','R','A','C')]=(char*)"diracenc";
    	*encs[CV_FOURCC('X','V','I','D')]=(char*)"xvidenc";
    	*encs[CV_FOURCC('X','2','6','4')]=(char*)"x264enc";
    	*encs[CV_FOURCC('M','P','1','V')]=(char*)"mpeg2enc";*/


		//the default one is not supported by Gstreamer
		//outputVideo = new VideoWriter(m_videoFilePath, denaturedVideoStream->getInputVideoCodec(), denaturedVideoStream->getVideoFrameRate(), denaturedVideoStream->getVideoFrameSize(), true);

		std::cout << "frame rate used for encoding is " << denaturedVideoStream->getVideoFrameRate() << std::endl;
		outputVideo = new VideoWriter(m_videoFilePath, CV_FOURCC('X', 'V', 'I', 'D'), denaturedVideoStream->getVideoFrameRate()/decoding_sample_rate, denaturedVideoStream->getVideoFrameSize(), true);

		//mpeg4 codec, unsupported
		//outputVideo = new VideoWriter(m_videoFilePath, CV_FOURCC('D', 'I', 'V', 'X'), denaturedVideoStream->getVideoFrameRate(), denaturedVideoStream->getVideoFrameSize(), true);

		//not supported
		//outputVideo = new VideoWriter(m_videoFilePath, CV_FOURCC('X', '2', '6', '4'), denaturedVideoStream->getVideoFrameRate(), denaturedVideoStream->getVideoFrameSize(), true);

		//OpenCV Error: Unspecified error (GStreamer: cannot link elements
		//) in CvVideoWriter_GStreamer::open, file /home/yuxiao/software/OpenCV-2.4.2/modules/highgui/src/cap_gstreamer.cpp, line 525
		//outputVideo = new VideoWriter(m_videoFilePath, CV_FOURCC('D', 'R', 'A', 'C'), denaturedVideoStream->getVideoFrameRate(), denaturedVideoStream->getVideoFrameSize(), true);

		//supported
		//outputVideo = new VideoWriter(m_videoFilePath, CV_FOURCC('H', 'F', 'Y', 'U'), denaturedVideoStream->getVideoFrameRate()/2, denaturedVideoStream->getVideoFrameSize(), true);

		//unsupported
		//outputVideo = new VideoWriter(m_videoFilePath, CV_FOURCC('M', 'P', 'I', 'V'), denaturedVideoStream->getVideoFrameRate()/2, denaturedVideoStream->getVideoFrameSize(), true);

		if ((outputVideo == NULL) || (!outputVideo->isOpened())){
				std::cout << "cannot initialize " << m_videoFilePath << std::endl;
				return false;
		}

		std::cout << "Denatured video is saved to " << m_videoFilePath << std::endl;


	}
	else
		return false;


	/*StreamInformation* denaturedFrameStream = segmentInfo->getByOutputType(TYPE_DENATURED_FRAME);
	if (denaturedFrameStream != NULL){
		bWriteFrames = true;
		m_framesPath = denaturedFrameStream->getFilePath();
	}
	else
		bWriteFrames = false;*/


	std::cout << "encoding configuration done" << std::endl;

	return true;
}


void VideoEncoding::loadConfiguration(){

	json_error_t err_t;
	json_t* root = json_load_file("XMLFiles/config.json", 0, &err_t);
	json_t* dmAddr = NULL;
	json_t* dmPort = NULL;
	json_t* write = NULL;
	json_t* rate = NULL;
	json_t* processingRate= NULL;
	json_t* decodingRate = NULL;

	if (root != NULL){

		dmAddr = json_object_get(root, "DATAMANAGER_URI");

		dmPort = json_object_get(root, "DATAMANAGER_PORT");

		write = json_object_get(root, "WRITE_JPEG_FRAMES");

		rate = json_object_get(root, "ENCODING_SAMPLE_RATE");

		processingRate = json_object_get(root, "DETECTION_SAMPLE_RATE");

		decodingRate = json_object_get(root, "DECODING_SAMPLE_RATE");

	}

	if (dmAddr != NULL)
		host_ = json_string_value(dmAddr);
	else
		host_ = "127.0.0.1";

	if (dmPort != NULL)
		port_ = atoi(json_string_value(dmPort));
	else
		port_ = 5000;

	if ((write != NULL) && (boost::iequals(json_string_value(write), "true"))){
				bWriteFrames = true;
	}

	if (rate != NULL)
		sample_rate = json_integer_value(rate);
	else
		sample_rate = 1;

	if (processingRate != NULL)
		denature_sample_rate = json_integer_value(processingRate);
	else
		denature_sample_rate = 1;

	if (processingRate != NULL)
		decoding_sample_rate = json_integer_value(decodingRate);
	else
		decoding_sample_rate = 1;

	json_object_clear(dmAddr);
	json_object_clear(dmPort);
	json_object_clear(rate);
	json_object_clear(root);

}
