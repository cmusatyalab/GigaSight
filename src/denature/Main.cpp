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
#include "UploadServer/UploadManager.h"
#include <signal.h>
#include <pthread.h>
#include <fstream>
#include "ImageProcessing/AESEncryption.h"
#include "HTTPServer/EmulateAndroidClient.h"

void runTillStopped(){
	// Wait for signal indicating time to shut down.
	sigset_t wait_mask;
	sigemptyset(&wait_mask);
	sigaddset(&wait_mask, SIGINT);
	sigaddset(&wait_mask, SIGQUIT);
	sigaddset(&wait_mask, SIGTERM);
	pthread_sigmask(SIG_BLOCK, &wait_mask, 0);
	int sig = 0;
	sigwait(&wait_mask, &sig);

	/*std::ofstream logFile;
	logFile.open("log");
	logFile << "STOP SIGNAL received";
	logFile.close();*/
}

/*
 * Remember to copy XMLFiles/ folder to Debug/ or Release/. XMLFiles/ contains the config.xml and the classifiers needed for face detection and recognition.
 * Examples:
 * 1) ./VideoDenaturing 2 0 /mnt/segments/Videos/1080p/events 5000
 * decode the first 5000 frames of each video under /mnt/segments/Videos/1080p/events.
 * If want to decode all the frames, replace 5000 with -1.
 * 2) ./VideoDenaturing 2 2 /mnt/segments/Videos/1080p/events/ 5000 1
 * Run decoding, denaturing and encoding in sequence. In this case, face detection starts after all the decoded frames have been pushed to the rawImageQueue.
 * Due to the limit of memory size, we try not to run more than 100 frames. Here, we process the first 5000 frames, but we can set decoding_sample_rate to 100 in config.xml.
 * If you want to scale down the video resolution for denaturing, replace 1 with 2, 3 or other values.
 * 3) ./VideoDenaturing 2 3 /mnt/segements/Videos/1080p/events/ 5000 2
 * Run decoding, denaturing and encoding threads in parallel. The encoded file will be written to a MP4 file. If the NFS path is not available, it will just save to test under Debug/.
 * 4) ./VideoDenaturing 2 5 /mnt/segments/Videos/1080p/events/
 * encrypt all the files in the given folder. For each file, one encrypted file, one key file will be generated.
 * 5) ./VideoDenaturing 2 6 /mnt/segments/Videos/1080p/events/video1.mp4 keyFile
 * decrypt video1.mp4. The key is saved in keyFile.
 * 6) ./VideoDenaturing 2 7 Log/ result
 * read all the detection_stat_* files and write records into a file called result.
 * 7) ./VideoDenaturing 2
 * Just run it and set the number of denaturing threads to 2.
 * 8) ./VideoDenaturing 2 /mnt/segments/Videos/1080p/events 5000 0
 * For end-to-end perf test. One thread is emulating mobile client which sends REST msgs. When the segmentID, streamID are available, the personal VM will read one file from the given folder.
 * 2 here represents the number of denaturing threads, 5000 represents how many frames of each vides should be processed. 0 is just to state that it is for e2e test.
 */


int main(int argc, char *argv[]){

	//argv[1]: number of threads for image processing
	//argv[2]: 0 only decoding; 2 decoding, face detection and encoding in pipeline
	//argv[2]: 3 decoding+face detection+encoding. for 1 and 2, only detect faces but do not modify the images and do not write.
	//argv[3]: input file path
	//argv[4]: number of frames to process. If it is set to -1, then process until the end of the file.
	//argv[5]: scale. If it is set to 2, it  means the original frame is scaled down by 2, e.g. from 1920x1080 to 960x540
	//argv[6]: give the name list for face recognition. e.g. "Satya;Pieter;". Leave it empty if you don't want to run face recognition

	int number_of_thread = 1;

	if (argc > 1){
		number_of_thread = atoi(argv[1]);
		std::cout << number_of_thread << " threads will be created for face detection" << std::endl;
	}


	//if argc > 4 is only used for perf test
	if (argc > 3){

		int type = atoi(argv[2]);

		switch (type) {

		case 0: {
			//only decoding
			//argv[3] is the file path of videos.
			//argv[4] states how many frames to decode for each video. If it is set to -1, it will decode all the available frames.
			boost::filesystem3::path p(argv[3]);
			VideoDecoding decoder(NULL, NULL, NULL, NULL, NULL);
			if (argc > 4)
				decoder.decodeFile(p, atoi(argv[4]));
			p.clear();
			return 0;
		}

		case 2: {

			if (argc < 4)
				return -1;

			//boost::filesystem3::path p(argv[3]);

			std::cout << "decoding, face detection and encoding are running in sequence" << std::endl;

			if (atoi(argv[4]) > 5000){
				std::cout << "SharedQueue may be over filled" << std::endl;
				return -1;
			}

			boost::filesystem3::path folder(argv[3]);

			if (!boost::filesystem3::exists(folder)){
					return -1;
			}

			boost::filesystem3::directory_iterator end_iter;

			if (boost::filesystem3::is_directory(folder)){

				for( boost::filesystem3::directory_iterator dir_iter(folder) ; dir_iter != end_iter ; ++dir_iter)
				{


					SharedQueue<std::pair<int, cv::Mat> > *rawImageQueue = new SharedQueue<std::pair<int, cv::Mat> >();
					ProcessingRuleContainer* ruleManager = new ProcessingRuleContainer();
					SharedQueue<std::pair<int, cv::Mat> > *processedImageQueue = new SharedQueue<std::pair<int, cv::Mat> >();

					int scale = 1;

					if (argc > 5){
							scale = atoi(argv[5]);
							std::cout << "Image width and height will be scaled down to " << 1/scale << "of original ones" << std::endl;
					}

					if (argc > 6){
							ruleManager->setFilteredInNames(argv[6]);
							std::cout << ruleManager->getNames() << "will be recognized" << std::endl;
							ruleManager->setFaceRecognition(true);
					}

					VideoDecoding decoder(rawImageQueue, NULL, NULL, NULL, NULL);

					boost::thread decodingThread(boost::bind(&VideoDecoding::decodeFile, &decoder, (*dir_iter).path(), atoi(argv[4])));

					decodingThread.join();


					boost::thread_group threadGroup;


					ImageProcessing* VideoProcessor[number_of_thread];
					for (int i=0; i<number_of_thread; i++){
							VideoProcessor[i]= new ImageProcessing(rawImageQueue, processedImageQueue, ruleManager, NULL);
							VideoProcessor[i]->setScale(scale);
							threadGroup.create_thread(boost::bind(&ImageProcessing::perfTestDetection, (ImageProcessing&)(*(VideoProcessor[i]))));
					}

					threadGroup.join_all();


					VideoEncoding videoEncoder(processedImageQueue, NULL);
					boost::thread encodingThread(boost::bind(&VideoEncoding::perfTest, &videoEncoder, atoi(argv[4])));

					encodingThread.join();
					std::cout << "test done" << std::endl;


					rawImageQueue->clean();
					if (rawImageQueue)
						delete rawImageQueue;


					processedImageQueue->clean();

					delete ruleManager;

					for (int i=0; i<number_of_thread; i++){
						delete VideoProcessor[i];
					}


				}//for

			folder.clear();
			return 0;
		}//if


		SharedQueue<std::pair<int, cv::Mat> > *rawImageQueue = new SharedQueue<std::pair<int, cv::Mat> >();
		ProcessingRuleContainer* ruleManager = new ProcessingRuleContainer();
		SharedQueue<std::pair<int, cv::Mat> > *processedImageQueue = new SharedQueue<std::pair<int, cv::Mat> >();

		int scale = 1;

		if (argc > 5){
				scale = atoi(argv[5]);
				std::cout << "Image width and height will be scaled down to " << 1/scale << "of original ones" << std::endl;
		}

		if (argc > 6){
				ruleManager->setFilteredInNames(argv[6]);
				std::cout << ruleManager->getNames() << "will be recognized" << std::endl;
				ruleManager->setFaceRecognition(true);
		}

		VideoDecoding decoder(rawImageQueue, NULL, NULL, NULL, NULL);

		boost::thread decodingThread(boost::bind(&VideoDecoding::decodeFile, &decoder, boost::filesystem3::path(argv[3]), atoi(argv[4])));

		decodingThread.join();


			boost::thread_group threadGroup;


			ImageProcessing* VideoProcessor[number_of_thread];
			for (int i=0; i<number_of_thread; i++){
				VideoProcessor[i]= new ImageProcessing(rawImageQueue, processedImageQueue, ruleManager, NULL);
				VideoProcessor[i]->setScale(scale);
				threadGroup.create_thread(boost::bind(&ImageProcessing::perfTestDetection, (ImageProcessing&)(*(VideoProcessor[i]))));
			}

			threadGroup.join_all();


			VideoEncoding videoEncoder(processedImageQueue, NULL);
			boost::thread encodingThread(boost::bind(&VideoEncoding::perfTest, &videoEncoder, atoi(argv[4])));

			encodingThread.join();
			std::cout << "test done" << std::endl;


			rawImageQueue->clean();
			if (rawImageQueue)
			delete rawImageQueue;


			processedImageQueue->clean();

			delete ruleManager;

			for (int i=0; i<number_of_thread; i++){
				delete VideoProcessor[i];
			}


			return 0;

		}

		case 3: {
			//decoding + processing + encoding in parellel

			std::cout << "decoding and face detection are running in parallel" << std::endl;
			//decoding + face detection
			boost::filesystem3::path folder(argv[3]);

			if (!boost::filesystem3::exists(folder)){
					return -1;
			}

			boost::filesystem3::directory_iterator end_iter;

			if (boost::filesystem3::is_directory(folder)){

				for( boost::filesystem3::directory_iterator dir_iter(folder) ; dir_iter != end_iter ; ++dir_iter)
				{

					SharedQueue<std::pair<int, cv::Mat> > *rawImageQueue = new SharedQueue<std::pair<int, cv::Mat> >();
					ProcessingRuleContainer* ruleManager = new ProcessingRuleContainer();
					SharedQueue<std::pair<int, cv::Mat> > *processedImageQueue = new SharedQueue<std::pair<int, cv::Mat> >();

					int scale = 1;

					if (argc > 5){
							scale = atoi(argv[5]);
							std::cout << "Image width and height will be scaled down to " << 1/scale << "of original ones" << std::endl;
					}

					if (argc > 6){
							ruleManager->setFilteredInNames(argv[6]);
							std::cout << ruleManager->getNames() << "will be recognized" << std::endl;
							ruleManager->setFaceRecognition(true);
					}

					if (argc < 4)
						break;

					VideoDecoding decoder(rawImageQueue, NULL, NULL, NULL, NULL);
					boost::thread_group threadGroup;
					threadGroup.create_thread(boost::bind(&VideoDecoding::decodeFile, &decoder, (*dir_iter).path(), atoi(argv[4])));

					ImageProcessing* VideoProcessor[number_of_thread];
					for (int i=0; i<number_of_thread; i++){
							VideoProcessor[i]= new ImageProcessing(rawImageQueue, processedImageQueue, ruleManager, NULL);
							VideoProcessor[i]->setScale(scale);
							threadGroup.create_thread(boost::bind(&ImageProcessing::perfTestDetection, (ImageProcessing&)(*(VideoProcessor[i]))));
					}

					VideoEncoding videoEncoder(processedImageQueue, NULL);
					threadGroup.create_thread(boost::bind(&VideoEncoding::perfTest, &videoEncoder, atoi(argv[4])));

					threadGroup.join_all();
					std::cout << "test done" << std::endl;


					rawImageQueue->clean();
					if (rawImageQueue)
						delete rawImageQueue;


					processedImageQueue->clean();

					delete ruleManager;

					for (int i=0; i<number_of_thread; i++)
						delete VideoProcessor[i];


				}

				folder.clear();

				return 0;

			}

			SharedQueue<std::pair<int, cv::Mat> > *rawImageQueue = new SharedQueue<std::pair<int, cv::Mat> >();
			ProcessingRuleContainer* ruleManager = new ProcessingRuleContainer();
			SharedQueue<std::pair<int, cv::Mat> > *processedImageQueue = new SharedQueue<std::pair<int, cv::Mat> >();

			int scale = 1;

			if (argc > 5){
				scale = atoi(argv[5]);
				std::cout << "Image width and height will be scaled down to " << 1/scale << "of original ones" << std::endl;
			}

			if (argc > 6){
				ruleManager->setFilteredInNames(argv[6]);
				std::cout << ruleManager->getNames() << "will be recognized" << std::endl;
				ruleManager->setFaceRecognition(true);
			}

			if (argc < 4)
				break;

			VideoDecoding decoder(rawImageQueue, NULL, NULL, NULL, NULL);
			boost::thread_group threadGroup;
			threadGroup.create_thread(boost::bind(&VideoDecoding::decodeFile, &decoder, folder, atoi(argv[4])));

			ImageProcessing* VideoProcessor[number_of_thread];
			for (int i=0; i<number_of_thread; i++){
					VideoProcessor[i]= new ImageProcessing(rawImageQueue, processedImageQueue, ruleManager, NULL);
					VideoProcessor[i]->setScale(scale);
					threadGroup.create_thread(boost::bind(&ImageProcessing::perfTestDetection, (ImageProcessing&)(*(VideoProcessor[i]))));
			}

			VideoEncoding videoEncoder(processedImageQueue, NULL);
			threadGroup.create_thread(boost::bind(&VideoEncoding::perfTest, &videoEncoder, atoi(argv[4])));

			threadGroup.join_all();
			std::cout << "test done" << std::endl;


			rawImageQueue->clean();
			if (rawImageQueue)
				delete rawImageQueue;


			processedImageQueue->clean();

			delete ruleManager;

			for (int i=0; i<number_of_thread; i++)
				delete VideoProcessor[i];

			folder.clear();


			return 0;
		}
		case 5:{

			//encrypt the whole video
			AESEncryption processor;
			boost::filesystem3::path p(argv[3]);
			boost::filesystem3::path q("");
			unsigned char key_data[32];
			strcpy((char*)key_data,  "PI=3.1415926...");
			//processor.aes_encrypt_file(p,key_data, q);


			boost::thread encryptionThread(boost::bind(&AESEncryption::aes_encrypt_file, &processor, p, key_data, q));

			encryptionThread.join();

			p.clear();
			q.clear();

			return 0;
		}
		case 6:{

			//decrypt a video file
			AESEncryption processor;
			boost::filesystem3::path p(argv[3]);
			boost::filesystem3::path log("");

			if (argc > 4){
				boost::filesystem3::path q(argv[4]);
				processor.aes_decrypt_file(p, q, log);
			}
			else{

				boost::filesystem3::path q("");
				processor.aes_decrypt_file(p, q, log);
			}


			return 0;

		}
		case 7:{
			//process data.e.g. merge all the logs in detection_stat_* into one file
			boost::filesystem3::path folder(argv[3]);

			if (!boost::filesystem3::exists(folder)){
				return -1;
			}

			std::string outputFileName = "";
			if (argc > 4)
				outputFileName.assign(argv[4]);
			else
				outputFileName.assign("result");

			std::ofstream outputStream;
			outputStream.open(outputFileName.c_str(), std::ios::out|std::ios::app);

			if (!outputStream.is_open()){
				std::cout << "output file cannot be opened" << std::endl;
				return -1;
			}

			boost::filesystem3::directory_iterator end_iter;

			if (boost::filesystem3::is_directory(folder)){

				for( boost::filesystem3::directory_iterator dir_iter(folder) ; dir_iter != end_iter ; ++dir_iter)
				{

					if ((*dir_iter).path().string().find("detection_stat_") != (*dir_iter).path().string().npos){

						std::ifstream inputFile;
						std::cout << "open " <<(*dir_iter).path().string()<< std::endl;
						inputFile.open((*dir_iter).path().c_str(), std::ios::in);

						if (!inputFile.is_open()){
							std::cout << "cannot open " << (*dir_iter).path().c_str() << std::endl;
							continue;
						}

						char buffer[1024];
						memset(buffer, 0, 1024);
						inputFile.read(buffer, 1024);

						outputStream << buffer;

						inputFile.close();
					}
				}
			}//if

			outputStream.close();
			return 0;

		}

	}//switch
	}//if argc > 3



	//initialize two sharedqueue, one for raw frames, and the other for denatured frames
	SharedQueue<std::pair<int, cv::Mat> > *rawImageQueue = new SharedQueue<std::pair<int, cv::Mat> >();

	SharedQueue<std::pair<int, cv::Mat> > *processedImageQueue = new SharedQueue<std::pair<int, cv::Mat> >();
	
	SharedQueue<std::pair<std::string, std::string> > *encryptionQueue = new SharedQueue<std::pair<std::string, std::string> >();

	//initialize three sharedqueue.
	//segmentList is used for informing the VideoDecoding thread
	SharedQueue<SegmentInformation*> *segmentList = new SharedQueue<SegmentInformation*>();

	// decoding thread will inform the VideoProcessing thread
	SharedQueue<SegmentInformation*> *processingSegmentList = new SharedQueue<SegmentInformation*>();

	//decoding thread will inform the VideoEncoding thread
	SharedQueue<SegmentInformation*> *encodingSegmentList = new SharedQueue<SegmentInformation*>();

	//container of privacy rules. Rules are added in request_handler().
	ProcessingRuleContainer* ruleManager = new ProcessingRuleContainer();

	// Block all signals for background threads.
	sigset_t new_mask;
	sigfillset(&new_mask);
	sigset_t old_mask;
	pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);

	boost::thread_group threadGroup;

	//new a thread for video decoding
	VideoDecoding videoDecoder(rawImageQueue, segmentList, processingSegmentList, encodingSegmentList, encryptionQueue);

	if (argc > 3){
		videoDecoder.enableE2ETest();
	}


	if (argc > 2){

		std::cout << "will process the first " << atoi(argv[2]) << "frames of each video" << std::endl;
		videoDecoder.setLimit(atoi(argv[2]));
	}

	boost::thread decodingThread(boost::bind(&VideoDecoding::operator(), &videoDecoder));


	ImageProcessing* VideoProcessor[number_of_thread];


	for (int i=0; i<number_of_thread; i++){
		VideoProcessor[i]= new ImageProcessing(rawImageQueue, processedImageQueue, ruleManager, processingSegmentList);
		threadGroup.create_thread((ImageProcessing&)(*(VideoProcessor[i])));
	}



	VideoEncoding videoEncoder(processedImageQueue, encodingSegmentList);

	//boost::thread encodingThread(videoEncoder);
	threadGroup.create_thread(videoEncoder);


	//run UploadServer in background thread
	upload::UploadManager& upMan = upload::UploadManager::Instance(segmentList);
	threadGroup.create_thread(boost::bind(&upload::UploadManager::run, &upMan));


	http::server::server s(segmentList, ruleManager);
	threadGroup.create_thread(boost::bind(&http::server::server::run, &s));

	EmulateAndroidClient *emulator = NULL;
	if ((argc > 3) && (atoi(argv[3]) == 0)){
		emulator = new EmulateAndroidClient();
		threadGroup.create_thread(boost::bind(&EmulateAndroidClient::operator(), emulator));
	}

	AESEncryption encryptor;
	threadGroup.create_thread(boost::bind(&AESEncryption::run, &encryptor, encryptionQueue));

	// Restore previous signals.
	pthread_sigmask(SIG_SETMASK, &old_mask, 0);

	//now run until CTRL-C
	runTillStopped();


	videoDecoder.stopDecoding();

	//use empty segmentInformation to inform videodecoding thread to stop
	SegmentInformation *emptySegment= new SegmentInformation();
	segmentList->push(emptySegment);

	for (int i=0; i< number_of_thread; i++)
		VideoProcessor[i]->stop();

	videoEncoder.stop();
	s.stop();
	upMan.stop();
	encryptor.stop();

	//use empty mat to inform imageprocessing and videoencoding threads to stop
	Mat emptyMat;
	for (int i=0; i< number_of_thread; i++)
		rawImageQueue->push(std::pair<int, cv::Mat>(0, emptyMat));

	processedImageQueue->push(std::pair<int, cv::Mat>(0, emptyMat));


	sleep(5);

	//pop up the images left in the queues
	rawImageQueue->clean();
	processedImageQueue->clean();
	segmentList->clean();
	processingSegmentList->clean();


}


