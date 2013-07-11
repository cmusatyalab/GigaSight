/*
 * ImageProcessing.cpp
 *
 *  Created on: Oct 4, 2012
 *      Author: yuxiao
 *  reference: http://www.opencv.org.cn/opencvdoc/2.3.1/html/doc/tutorials/objdetect/cascade_classifier/cascade_classifier.html#cascade-classifier.
 *  http://saju.net.in/code/misc/openssl_aes.c.txt
 */

#include "ImageProcessing.h"
#include <boost/filesystem.hpp>
#include <jansson.h>
#include "../HTTPServer/Client.h"
#include <boost/algorithm/string.hpp>



using namespace cv;
using namespace std;




ImageProcessing::ImageProcessing(SharedQueue< std::pair<int, cv::Mat> >* readQueue){

	inputQueue = readQueue;
	outputQueue = NULL;
	ruleManager = NULL;
	segmentQueue = NULL;

	recognizer_id = LBPH;
	bRecogEnabled = false; //use face recognition or not
	bEnding = false;
	currentSegment = NULL;
	bLocEnabled = false;
	bTimeEnabled = false;

	scale = 1;

	min_area = 0;
	frontal_detector = FRONTAL_LBP;
}


ImageProcessing::ImageProcessing(SharedQueue< std::pair<int, cv::Mat> >* readQueue, SharedQueue<std::pair<int,cv::Mat> >* writeQueue, ProcessingRuleContainer* rules, SharedQueue<SegmentInformation*>* segmentList){

	inputQueue = readQueue;
	outputQueue = writeQueue;
	ruleManager = rules;
	segmentQueue = segmentList;

	recognizer_id = LBPH;
	bRecogEnabled = false; //use face recognition or not
	bEnding = false;
	currentSegment = NULL;
	bLocEnabled = false;
	bTimeEnabled = false;

	loadConfiguration();

	model = NULL;

	resolution_id = NOT_SUPPORTED;

	scale = 1;

	min_area = 0;


	frontal_detector = FRONTAL_LBP;
}


ImageProcessing::~ImageProcessing() {
	// TODO Auto-generated destructor stub
	//eyes_cascade.~CascadeClassifier();
	//face_cascade.~CascadeClassifier();
	profile_cascade_name.clear();
	face_cascade_name.clear();
	host_.clear();
	labelMap.clear();

}

bool ImageProcessing::preprocessing(){

	std::cout << "PREPROCESSING" << std::endl;

	if (currentSegment == NULL)
		return false;

	StreamInformation* gpsStream = currentSegment->getByOutputType(TYPE_GPS_STREAM);
	std::string gpsFilePath;

	if (currentSegment->isGPSAvailable()){

		gpsFilePath = gpsStream->getLocalFilePath();

		Client c;

		json_t* request = json_object();

		json_object_set_new(request, "location", json_string(ruleManager->parseGPSRange(gpsFilePath).c_str()));

		c.updateSegmentInformation(host_, port_, currentSegment->getSegmentID(), json_dumps(request, 0));

		std::cout << "update location information " << json_dumps(request, 0) << std::endl;
	}

	if ((ruleManager->isLocationScopeEnabled()) && (currentSegment->isGPSAvailable())){

		std::cout << "LOCATION SCOPE" << std::endl;
		bLocEnabled = true;

		StreamInformation* oriVideo = currentSegment->getByOutputType(TYPE_ORIGINAL_VIDEO);


		if ((gpsStream != NULL) && (oriVideo != NULL)){
				double fps = oriVideo->getVideoFrameRate();
				ruleManager->parseFrameRangeByGPS(gpsFilePath, fps);
		}

	}

	bTimeEnabled = ruleManager->isTimeScopeEnabled();


	if (bTimeEnabled){

		std::cout << "TIME SCOPE" << std::endl;

		StreamInformation *oriVideo = currentSegment->getByOutputType(TYPE_ORIGINAL_VIDEO);

		if (oriVideo != NULL){

			double duration = oriVideo->getDuration();
			std::string startTime = oriVideo->getStartTime();
			double fps = oriVideo->getVideoFrameRate();

			ruleManager->parseFrameRangeByTime(startTime, duration, fps);
		}

	}

	//remove model == NULL here
	if (ruleManager->isFaceRecogEnabled()){
		std::cout << "FACE SCOPE" << std::endl;

		StreamInformation *oriVideo = currentSegment->getByOutputType(TYPE_ORIGINAL_VIDEO);

		if (oriVideo != NULL){

				double width = oriVideo->getVideoFrameSize().width;

				if (width == 320)
					resolution_id = LOW_320P;
				else if (width == 1920)
					resolution_id = HIGH_1920P;
				else if (width == 720)
					resolution_id = MEDIUM_720P;
				else if (width == 1080)
					resolution_id = HIGH_1080P;
				else
					resolution_id = NOT_SUPPORTED;

		}

		return trainRecognizer(recognizer_id);
	}


	return true;

}

bool ImageProcessing::loadFaceRecognitionModels(std::string folderPath){

	switch (recognizer_id) {
		case FISHER:{
			boost::filesystem3::path p(folderPath+"fisher");

			if (boost::filesystem3::exists(p)){
				model = createFisherFaceRecognizer();
				model->load(folderPath + "fisher");
				return true;
			}

			break;
		}
		case EIGEN:{

			boost::filesystem3::path p(folderPath + "eigen");

			if (boost::filesystem3::exists(p)){
				model = createEigenFaceRecognizer();
				model->load(folderPath + "eigen");
				return true;
			}

			break;
		}
		case LBPH:{
			boost::filesystem3::path p(folderPath + "lbph");

			if (boost::filesystem3::exists(p)){
				model = createLBPHFaceRecognizer();
				model->load(folderPath + "lbph");
				return true;
			}

			break;
		}
	}

	return false;

}


//when the thread starts, it first run operator()()
void ImageProcessing::operator()(){

	int prevIndex = -1;
	int frameCount = 0;

	//load two XML files for face detection
	if (!initFaceDetection()){
		cout << "Cascade classifiers cannot be initialized" << endl;
		return;
	}

	std::ofstream logFile;
	boost::posix_time::ptime mst1, mst2;
	boost::posix_time::time_duration msdiff;

	std::string fileName;
	std::ostringstream x;
	x << (unsigned int)pthread_self();
	fileName.assign("Log/detection_frame_" + x.str());

	x.clear();

	std::cout << "write frame log to " << fileName <<  std::endl;



	//Mat rawImage;
	std::pair<int, Mat> rawImage;
	ENUM_ACTION_TYPE action;


	while (!bEnding){

		std::cout << "waiting for raw frames to arrive for processing "<< std::endl;
		inputQueue->wait_and_pop(rawImage);

			//std::cout << "Received the 1st frame for processing" << std::endl;

			/*if (segmentQueue->try_pop(currentSegment))
				preprocessing();
			else
				std::cout << "cannot find currentSegment" << std::endl;
		*/

			//to support multi-threading

		if (prevIndex == -1){
			segmentQueue->read_first_value(currentSegment);

			if (!preprocessing()){
				std::cout << "preprocessing failed" << std::endl;
				break;
			}

			mst1 = boost::posix_time::microsec_clock::local_time();

			logFile.open(fileName.c_str(), std::ios::out | std::ios::app);

			logFile << mst1 << ";" << "\r\n";

			if (!logFile.is_open())
				continue;

		}

		if (!rawImage.second.empty()){


			mst1 = boost::posix_time::microsec_clock::local_time();
			logFile << mst1 << ";";
			//outputQueue->push(processFaces(rawImage, false));

			//this may cause errors if the total number of frames are less than the number of threads.
			if (prevIndex >= rawImage.first){

				std::cout << "it has been stopped by other threads" << std::endl;

				segmentQueue->read_first_value(currentSegment);

				if (!preprocessing())
					break;

				frameCount = 0;

			}

			++frameCount;


			if (ruleManager->getRuleCount() == 0){

				outputQueue->push(std::pair<int, cv::Mat>(rawImage.first, processFaces(rawImage.second, false)));

				prevIndex = rawImage.first;
			}
			else {

				action = filteredIn(rawImage.first);

				switch (action){
					case ACTION_NOT_SPECIFIED:
						outputQueue->push(std::pair<int, cv::Mat>(rawImage.first, processFaces(rawImage.second, false)));
						break;
					case REMOVE_FACE:
						outputQueue->push(std::pair<int, cv::Mat>(rawImage.first, processFaces(rawImage.second, false)));
						break;
					case NO_ACTION:
						outputQueue->push(std::pair<int, cv::Mat>(rawImage.first, rawImage.second.clone()));
						break;
					case BLUR_IMAGE:{

						break;

						}
					case DELETE_FRAME:{
						//currently, for time and location filters, use this time
						Mat dst;

						cv::blur(rawImage.second, dst, rawImage.second.size());

						//for (int i = 1; i < MAX_KERNEL_LENGTH; i = i + 2 )
							//bilateralFilter (rawImage, dst, i, i*2, i/2 );

						//for (int i = 1; i < MAX_KERNEL_LENGTH; i = i + 2 )
							//	cv::GaussianBlur(rawImage, dst, cv::Size(i, i),0, 0);

						outputQueue->push(std::pair<int, cv::Mat>(rawImage.first, dst));
						break;
					}
					case BLUR_AND_REMOVE_FACE:{
						outputQueue->push(std::pair<int, cv::Mat>(rawImage.first, processFaces(rawImage.second, true)));
						break;
					}
				}

				prevIndex = rawImage.first;


			}

			mst2 = boost::posix_time::microsec_clock::local_time();
			msdiff = mst2 - mst1;


			logFile << mst2 << ";" << rawImage.first << ";" << msdiff.total_milliseconds() << ";" << "\r\n";


			if (frameCount%10 == 0)
				std::cout<<frameCount << " frames processed by this thread"<<std::endl;


			while (outputQueue->size() > 50){
				sleep(1);
			}
			//std::cout << "outputQueue size" << outputQueue->size() << std::endl;
		}
		else {
			//push an empty image to the queue to inform decodingthread that a new stream starts
			outputQueue->push(std::pair<int, cv::Mat>(rawImage.first, rawImage.second));

			std::cout<<frameCount << " frames processed before empty frame detected."<<std::endl;

			//the following is for multi-threading
			//segmentQueue->try_pop(currentSegment);

			prevIndex = -1;

			mst2 = boost::posix_time::microsec_clock::local_time();

			if (logFile.is_open()){
				logFile << mst2 << ";" << frameCount << ";" << "\r\n";
				logFile.close();
			}

			frameCount = 0;


		}

	}


	if (logFile.is_open())
		logFile.close();

	std::cout << "ImageProcessor stops" << std::endl;

}

void ImageProcessing::stop(){
	bEnding = true;
}

//Load classifiers
bool ImageProcessing::initFaceDetection(){
    
	//std::cout << "initFaceDetection" <<std::endl;

	//face_cascade_name = "XMLFiles/haarcascade_frontalface_alt_tree.xml";
	switch (frontal_detector){

	case FRONTAL_HAAR:
		face_cascade_name = "XMLFiles/haarcascade_frontalface_alt.xml";
		break;
	case FRONTAL_LBP:
		face_cascade_name = "XMLFiles/lbpcascade_frontalface.xml";
		break;

	}

	//face_cascade_name = "XMLFiles/haarcascade_profileface.xml";
    //eyes_cascade_name = "XMLFiles/haarcascade_eye_tree_eyeglasses.xml";
    
    profile_cascade_name = "XMLFiles/haarcascade_profileface.xml";
   //load a .xml classifier file. It can be either a Haar or a LBP classifier.

    if(!face_cascade.load(face_cascade_name)){
        cout << "Failed to load "<<face_cascade_name<< endl;
        return false;
    }
    
    if(!profile_cascade.load(profile_cascade_name)){
        cout << "Failed to load "<<profile_cascade_name<< endl;
        return false;
    }



    json_error_t err_t;
    json_t* root = json_load_file("Test0|CV_HAAR_SCALE_IMAGEFiles/label.json", 0, &err_t);
    json_t* label = NULL;
    json_t* name = NULL;
    json_t* persons = NULL;

    if (root != NULL)
    	persons = json_object_get(root, "namelist");

    if (persons != NULL){

    	if (json_is_array(persons)){

    		for (unsigned int j=0; j<json_array_size(persons); j++){
    			label = json_object_get(json_array_get(persons, j), "label");
    			name = json_object_get(json_array_get(persons, j), "name");

    			labelMap.insert(std::pair<int, std::string>(json_integer_value(label), json_string_value(name)));
    		}
    	}
    }


    //std::cout << labelMap.size() << "persons images available"<< std::endl;


    delete persons;
    delete name;
    delete label;
    delete root;

	return true;
}


int ImageProcessing::recognizeFaces(Mat& rawImage, int scale, int frameIndex){

	vector<Rect> faces, profiles;
	Mat frame_gray;
	//Mat frame_rotate;
	int faceSize = 0;

	if (scale != 1) {

		Mat tempFrame(cvSize(rawImage.size().width/scale, rawImage.size().height/scale), rawImage.depth(), rawImage.channels());

		//std::stringstream index;
		//index << frameIndex;

		cv::resize(rawImage, tempFrame, tempFrame.size(), 0, 0, INTER_LINEAR);

		//imwrite("scaledImages/"+index.str()+".jpeg", tempFrame);//for testing

		cvtColor(tempFrame, frame_gray, CV_BGR2GRAY);

	} else
	//convert the original image to gray scale
		cvtColor(rawImage, frame_gray, CV_BGR2GRAY);

	//Apply Histogram Equalization which improves the contrast in an image.
	equalizeHist(frame_gray, frame_gray);

	//-- Detect faces
	/*void CascadeClassifier::detectMultiScale(const Mat& image, vector<Rect>& objects,
	*  double scaleFactor=1.1, int minNeighbors=3, int flags=0, Size minSize=Size())
	*  scaleFactor must be bigger than 1
	*/
	//face_cascade.detectMultiScale(frame_gray, faces, 1.1, 1, 0|CV_HAAR_SCALE_IMAGE, cv::Size(min_area, min_area));

	//eyes_cascade.detectMultiScale(frame_gray, profiles, 1.1, 1, 0|CV_HAAR_SCALE_IMAGE, Size(min_area, min_area));

	face_cascade.detectMultiScale(frame_gray, faces, 1.1, min_neighbour, flags, Size(min_area, min_area));

	profile_cascade.detectMultiScale(frame_gray, profiles, 1.1, min_neighbour, flags, Size(min_area, min_area));

	//int maxIndex = frameIndex + sample_rate -1;

	//std::cout << "maxIndex = " << maxIndex << std::endl;

	if (!ruleManager->isFaceRecogEnabled()){

		for(unsigned int i = 0; i < faces.size(); i++){

			//Rect face_i = faces[i];

			Rect face_i(faces[i].x*scale, faces[i].y*scale, faces[i].width*scale, faces[i].height*scale);
			//draw a rectangle around the detected face
			//rectangle(rawImage, face_i, CV_RGB(0,255,0), CV_FILLED);
			rectangle(rawImage, face_i, CV_RGB(0,255,0), CV_FILLED);
		}

		for(unsigned int i = 0; i < profiles.size(); i++){
			//Rect face_i = faces[i];

			Rect face_i(profiles[i].x*scale, profiles[i].y*scale, profiles[i].width*scale, profiles[i].height*scale);

			//draw a rectangle around the detected face
			//rectangle(rawImage, face_i, CV_RGB(255,0,0), 5);
			rectangle(rawImage, face_i, CV_RGB(0,255,0), CV_FILLED);
		}

		if (outputQueue != NULL)
			outputQueue->push(std::pair<int, Mat>(frameIndex, rawImage));


		/*std::pair<int, Mat> nextFrame;
		if (frameIndex < maxIndex)
			inputQueue->wait_and_pop(nextFrame);
		else {
			frame_gray.release();
			//frame_rotate.release();
			faceSize = faces.size()+profiles.size();
			faces.clear();
			profiles.clear();
			return faceSize;
		}

		while (frameIndex <= maxIndex){

			//this is just for one-denaturing-thread case now


			frameIndex = nextFrame.first;
			std::cout << "next one is " << frameIndex << std::endl;

			for(unsigned int i = 0; i < faces.size(); i++){

				Rect face_i(faces[i].x*scale, faces[i].y*scale, faces[i].width*scale, faces[i].height*scale);
				//draw a rectangle around the detected face
				//rectangle(rawImage, face_i, CV_RGB(0,255,0), CV_FILLED);
				rectangle(nextFrame.second, face_i, CV_RGB(0,255,0), CV_FILLED);
			}

			for(unsigned int i = 0; i < profiles.size(); i++){
				//Rect face_i = faces[i];

				Rect face_i(profiles[i].x*scale, profiles[i].y*scale, profiles[i].width*scale, profiles[i].height*scale);

				//draw a rectangle around the detected face
				//rectangle(rawImage, face_i, CV_RGB(255,0,0), 5);
				rectangle(nextFrame.second, face_i, CV_RGB(0,255,0), CV_FILLED);
			}

			if (outputQueue != NULL)
				outputQueue->push(std::pair<int, Mat>(nextFrame.first, nextFrame.second.clone()));

			std::cout << "outputQueue size " << outputQueue->size() << std::endl;

			if (frameIndex == maxIndex)
				break;

			inputQueue->wait_and_pop(nextFrame);

		}//while
		*/


		//std::stringstream index;
		//index << frameIndex;
		//imwrite("Images-d/"+index.str()+".jpeg", rawImage);

		frame_gray.release();
		//frame_rotate.release();
		faceSize = faces.size()+profiles.size();
		faces.clear();
		profiles.clear();
		return faceSize;
	}

  	int im_width = rawImage.cols;
  	int im_height = rawImage.rows;

  	for(unsigned int i = 0; i < faces.size(); i++){

		Rect face_i = faces[i];
		int prediction;


		if (recognizer_id != LBPH){
				Mat face_resized;

				// Crop the face from the image. So simple with OpenCV C++:
				Mat face;

				face = frame_gray(face_i);

				resize(face, face_resized, Size(im_width, im_height), 1.0, 1.0, INTER_CUBIC);

				if ((im_width == trainedImageWidth) && (im_height == trainedImageHeight))
					prediction = model->predict(face_resized);
				else{
						std::cout << "The trained and tested images should of the same size" << std::endl;
						std::cout << "Trained Image Height: " << trainedImageHeight << " width: " << trainedImageWidth << std::endl;
						std::cout << "Tested image height: " << im_height << " width: " << im_width << std::endl;
						break;
				}

			}
			else
				 prediction = model->predict(frame_gray);


		   //std::cout << "prediction " << prediction << std::endl;

		   //prediction = the id of a person in training data set.
		  //are we going to remove the faces of all the persons included in the training set
		   if (!validateResult(prediction)){
				std::cout << "recognized face is not listed in the filter list" << std::endl;
				continue;
		   }

		   for(unsigned int i = 0; i < faces.size(); i++){

		   	  	Rect face_i(faces[i].x*scale, faces[i].y*scale, faces[i].width*scale, faces[i].height*scale);

		   	  	//draw a rectangle around the detected face
		   	  	rectangle(rawImage, face_i, CV_RGB(0,255,0), CV_FILLED);
		   }

		   if (outputQueue != NULL)
			   outputQueue->push(std::pair<int, Mat>(frameIndex, rawImage));

  	}

	return faces.size();

}


Mat& ImageProcessing::processFaces(Mat& rawImage, bool bBlur){

	vector<Rect> faces, profiles;
	Mat frame_gray;
	bool bRotate = false;
	Mat frame_rotate;

	//convert the original image to gray scale
	cvtColor(rawImage, frame_gray, CV_BGR2GRAY);
	
	//Apply Histogram Equalization which improves the contrast in an image.
	equalizeHist(frame_gray, frame_gray);

	//-- Detect faces
	/*void CascadeClassifier::detectMultiScale(const Mat& image, vector<Rect>& objects, 
	*  double scaleFactor=1.1, int minNeighbors=3, int flags=0, Size minSize=Size())
	*/
  //	face_cascade.detectMultiScale( frame_gray, faces, 1.1, 3, 0|CV_HAAR_SCALE_IMAGE);

	face_cascade.detectMultiScale(frame_gray, faces, 1.1, min_neighbour, flags, Size(min_area, min_area));

	profile_cascade.detectMultiScale(frame_gray, profiles, 1.1, min_neighbour, flags, Size(min_area, min_area));


  	if (faces.size()+profiles.size() > 0)
  		std::cout << faces.size()+profiles.size() << " faces detected" << std::endl;

  /*	if (faces.size() == 0){
  		//consider rotate the images and try again
  		//future work...
  		frame_rotate = Mat::zeros(rawImage.size(), rawImage.type());
  		 /// Compute a rotation matrix with respect to the center of the image
  		Point center = Point( rawImage.cols/2, rawImage.rows/2 );
  		double angle = -90.0;//clockwise
  		double scale = 1.0;

  		Mat rot_mat( 2, 3, CV_32FC1 );
  		rot_mat = getRotationMatrix2D( center, angle, scale );
  		warpAffine( frame_gray, frame_rotate, rot_mat, rawImage.size() );

  		face_cascade.detectMultiScale( frame_rotate, faces, 1.1, 3, 0|CV_HAAR_SCALE_IMAGE, cv::Size(min_area, min_area));


  		//if still cannot find a face, return the original mat
  		if (faces.size() == 0)
  			return rawImage;
  		else
  			bRotate = true;

  	}
*/

  	if (!ruleManager->isFaceRecogEnabled()){


  		//if we are going to remove all the faces from the photos,  no need to run face recognition
  		for(unsigned int i = 0; i < faces.size(); i++){

  			Rect face_i = faces[i];

  			//draw a rectangle around the detected face
  			rectangle(rawImage, face_i, CV_RGB(0,255,0), CV_FILLED);

  			/*std::cout << "rectangle " << face_i.size().height << ":" << face_i.size().width << std::endl;

  			rectangle(rawImage, face_i, CV_RGB(0,255,0));

  			Mat dst(rawImage.clone());
  			cv::blur(rawImage, dst, face_i.size(), cv::Point(face_i.x, face_i.y));
  			rawImage = dst;*/

  			//not tested yet
  			//for (int i = 1; i < MAX_KERNEL_LENGTH; i = i + 2 )
  			//cv::GaussianBlur(rawImage, rawImage.clone(), face_i.size(),0, 0);
  		}

  		for (unsigned int i=0; i< profiles.size(); i++){
  			Rect face_i = profiles[i];
  			rectangle(rawImage, face_i, CV_RGB(0,255,0), CV_FILLED);
  		}

  		frame_gray.release();
  		faces.clear();
  		profiles.clear();

 		return rawImage;
  	}


  	//std::cout << "face recognition" << std::endl;
  	//Conduct face recognition

  	int im_width = rawImage.cols;
  	int im_height = rawImage.rows;

  	for(unsigned int i = 0; i < faces.size(); i++){

  		Rect face_i = faces[i];
  		int prediction;


  		if (recognizer_id != LBPH){
  			Mat face_resized;

  			// Crop the face from the image. So simple with OpenCV C++:
  			Mat face;

  			if (!bRotate)
  				face = frame_gray(face_i);
  			else {
  				face = frame_rotate(face_i);
  				im_width = frame_rotate.cols;
  				im_height = frame_rotate.rows;
  			}

  			resize(face, face_resized, Size(im_width, im_height), 1.0, 1.0, INTER_CUBIC);

  			if ((im_width == trainedImageWidth) && (im_height == trainedImageHeight))
  				prediction = model->predict(face_resized);
  			else{
  				std::cout << "The trained and tested images should of the same size" << std::endl;
  				std::cout << "Trained Image Height: " << trainedImageHeight << " width: " << trainedImageWidth << std::endl;
  				std::cout << "Tested image height: " << im_height << " width: " << im_width << std::endl;
  				break;
  			}

  		}
  		else
  			 prediction = model->predict(frame_gray);


  		//std::cout << "prediction " << prediction << std::endl;

  		//prediction = the id of a person in training data set.
  		//are we going to remove the faces of all the persons included in the training set
  		if (!validateResult(prediction)){
  			std::cout << "recognized face is not listed in the filter list" << std::endl;
  			continue;
  		}

  		//if the face is recognized, do something here

  		// First of all draw a green rectangle around the detected face:
  		if (!bRotate){
  			rectangle(rawImage, face_i, CV_RGB(0, 255,0), CV_FILLED);
  		}
  		else {

  			 /// Compute a rotation matrix with respect to the center of the image
  			 Point center = Point( frame_rotate.cols/2, frame_rotate.rows/2 );
  			 double angle = -90.0;
  			 double scale = 1.0;

  			 Mat temp = Mat::zeros(rawImage.size(), rawImage.type());
  			 Mat rot_mat( 2, 3, CV_32FC1 );
  			 rot_mat = getRotationMatrix2D( center, angle, scale );
  			 warpAffine( rawImage, temp, rot_mat, rawImage.size() );

  			 rectangle(temp, face_i, CV_RGB(255,0,0), CV_FILLED);

    		 rawImage = Mat::zeros(frame_rotate.size(), frame_rotate.type());

  			 angle = 90.0;
  			 rot_mat = getRotationMatrix2D( center, angle, scale );
  			 warpAffine( temp, rawImage, rot_mat, temp.size() );
  		}


  	}


  	return rawImage;

}

void ImageProcessing::loadTrainingSet(const string& folderName, vector<Mat>& images, vector<int>& labels){

	boost::filesystem3::path p(folderName);

	boost::filesystem3::directory_iterator end_iter;

//	std::multimap<std::time_t, boost::filesystem3::path> result_set;

	//"user01/01.jpg" "user01/02.jpg"
	if (boost::filesystem3::exists(p) && boost::filesystem3::is_directory(p)){

		for( boost::filesystem3::directory_iterator dir_iter(p) ; dir_iter != end_iter ; ++dir_iter)
		{

			if (boost::filesystem3::is_directory((*dir_iter).status())){

				boost::filesystem3::directory_iterator end_iter_2;
				std::string label = (*dir_iter).path().stem().string();

				for( boost::filesystem3::directory_iterator dir_iter_2(*dir_iter) ; dir_iter_2 != end_iter_2 ; ++dir_iter_2){

					if (boost::filesystem3::is_regular_file((*dir_iter_2).status())){

						images.push_back(imread((*dir_iter_2).path().string(), 0));

						std::cout << (*dir_iter_2).path().string() << "label: " << label << std::endl;
						labels.push_back(atoi(label.c_str()));

					}

				}//for
			}


		}//for

	}

}

bool ImageProcessing::trainRecognizer(TYPE_RECOGNIZER type){

	// These vectors hold the images and corresponding labels:
    vector<Mat> images;
    vector<int> labels;
    std::string folderPath;
    
    //the .csv file should be created beforehand
    /*string cvsFileName = "training.csv";
    
    // Read in the data (fails if no valid input filename is given, but you'll get an error message):
    try {
        read_csv(cvsFileName, images, labels);
    } catch (cv::Exception& e) {
    	cerr << "Error opening file \"" << cvsFileName << "\". Reason: " << e.msg << endl;
        return false;
    }*/

    switch (resolution_id){
    case LOW_320P:{
    	folderPath.assign("XMLFiles/320P/");

    	if (loadFaceRecognitionModels(folderPath)){
    		trainedImageWidth = 320;
    		trainedImageHeight = 240;
    		return true;
    	}

    	loadTrainingSet("TestFiles/320P/", images, labels);

    	break;
    }
   case MEDIUM_720P:{

    	folderPath.assign("XMLFiles/720P/");

    	if (loadFaceRecognitionModels(folderPath)){
    		trainedImageWidth = 720;
    		trainedImageHeight = 480;
    		return true;
    	}

    	loadTrainingSet("TestFiles/720P/", images, labels);
    	break;
    }
    case HIGH_1080P:{

    	folderPath.assign("XMLFiles/1080P/");

    	if (loadFaceRecognitionModels(folderPath)){
    		trainedImageWidth = 1080;
    		trainedImageHeight = 720;
    		return true;
    	}

    	loadTrainingSet("TestFiles/1080P/", images, labels);
    	break;
    }
    case HIGH_1920P:{
    	folderPath.assign("XMLFiles/1920P/");

    	 if (loadFaceRecognitionModels(folderPath)){
    	  		trainedImageWidth = 1920;
    	  		trainedImageHeight = 1080;
    	  		return true;
    	 }

    	 loadTrainingSet("TestFiles/1920P/", images, labels);
    	 break;

    }
    default:
    	//training data set is missing
    	return false;
    }



    if(images.size() <= 1) {
         std::cout<< "This demo needs at least 2 images to work. Please add more images to your data set!"<<std::endl;
         return false;
    }


    switch (type){
		case FISHER:{
			// Create a FaceRecognizer and train it on the given images:
			model = createFisherFaceRecognizer();
			break;
		}
		case EIGEN:{
			 //Eigen face recognizer
			 model = createEigenFaceRecognizer();
			 break;
		}
		case LBPH:{
			model =  createLBPHFaceRecognizer();
			break;
		}
    }

    if (model != NULL){

    	std::cout << "start training.It will take a while" << std::endl;
    	model->train(images, labels);
    	std::cout << "training finished" << std::endl;


    	switch (type){
    		case FISHER:{
    				// Create a FaceRecognizer and train it on the given images:
    				cv::FileStorage fs(folderPath+"fisher", cv::FileStorage::WRITE);
    				model->save(fs);
    				break;
    			}
    			case EIGEN:{
    				 //Eigen face recognizer
    				cv::FileStorage fs(folderPath + "eigen", cv::FileStorage::WRITE);
    				model->save(fs);
    				 break;
    			}
    			case LBPH:{
    				cv::FileStorage fs(folderPath + "lbph", cv::FileStorage::WRITE);
    				model->save(fs);
    				break;
    			}
    	}
    }

    return true;
}



void ImageProcessing::perfTestDetection(){

	double frameCount = 0;
	bool bFirst = true;
	int faceCount = 0;

	std::pair<int, Mat> rawImage;
	std::ofstream logFile;
	std::ofstream statisticFile;
	boost::posix_time::ptime mst1, mst2, filestart, fileend;
	boost::posix_time::time_duration msdiff, msdiff2;


	if (!initFaceDetection())
		return;


	double totalDuration = 0.0;
	int prevIndex = 2;

	std::string fileName, fileName2;
	std::ostringstream x;
	x << (unsigned int)pthread_self();
	fileName.assign("Log/detection_frame_" + x.str());
	fileName2.assign("Log/detection_stat_"+x.str());
	x.clear();

	std::cout << "write frame log to " << fileName << "and statistics to" << fileName2 << std::endl;

	logFile.open(fileName.c_str(), std::ios::out | std::ios::app);
	statisticFile.open(fileName2.c_str(), std::ios::out | std::ios::app);

	if ((!logFile.is_open())|| (!statisticFile.is_open())){
		std::cout << "files cannot be opened" << std::endl;
		return;
	}


	inputQueue->wait_and_pop(rawImage);
	filestart = boost::posix_time::microsec_clock::local_time();

	statisticFile << filestart << ";";

	//when it starts, it continues receiving images until all the frames have been processed

	while (!rawImage.second.empty()){

			logFile << rawImage.first  << ";";

			mst1 = boost::posix_time::microsec_clock::local_time();

			logFile << mst1 << ";";

			if (rawImage.first < prevIndex){

					if (totalDuration > 0){
						statisticFile << fileend << ";" << totalDuration << ";" << frameCount << ";" << frameCount*1000/totalDuration << ";" << std::endl;
					}


					filestart = mst1;
					frameCount = 0;

					if (!bFirst)
						statisticFile << filestart << ";";

					bFirst = false;
			}


			++frameCount;


			//processFaces(rawImage.second, false);
			if (rawImage.first% sample_rate == 0){
				faceCount = recognizeFaces(rawImage.second, scale, rawImage.first);
			}

			mst2 = boost::posix_time::microsec_clock::local_time();
			msdiff = mst2 - mst1;

			logFile << mst2 << ";" << msdiff.total_milliseconds() << ";" << faceCount << ";"<< std::endl;

			totalDuration += msdiff.total_milliseconds();

			prevIndex = rawImage.first;

			//should not be commented while running as native processes
			//rawImage.second.release();

			//if (prevIndex%10 == 0)
			//	std::cout << prevIndex << stdsleep(1);::endl;


			/*if (!inputQueue->try_pop(rawImage)){
				std::cout << inputQueue->size() << "cannot fetch frame from the rawImageQueue" << std::endl;
				break;
			}*/


			while (outputQueue->size() > 50){
				sleep(1);
			}

			inputQueue->wait_and_pop(rawImage);

			//std::cout << "ref count " << *(rawImage.second.refcount) << std::endl;

		}

		fileend = mst2;
		msdiff2 = fileend-filestart;

		statisticFile << fileend << ";" << totalDuration << ";" << frameCount << ";" << frameCount*1000/totalDuration << ";" << std::endl;



    	logFile.close();

    	statisticFile.close();

    	fileName.clear();
    	fileName2.clear();

}


//return the number of faces
int ImageProcessing::detectFace(Mat& rawImage){

	Mat frame_gray;
	vector<Rect> faces;
	//vector<Rect> profiles;

	//convert the original image to gray scale
	cvtColor(rawImage, frame_gray, CV_BGR2GRAY);

	//Apply Histogram Equalization which improves the contrast in an image.
	equalizeHist(frame_gray, frame_gray);

	//-- Detect faces
	/*void CascadeClassifier::detectMultiScale(const Mat& image, vector<Rect>& objects,
	*  double scaleFactor=1.1, int minNeighbors=3, int flags=0, Size minSize=Size())
	*/
	face_cascade.detectMultiScale( frame_gray, faces, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE, Size(20, 20) );


	//std::cout << faces.size() << " faces detected" << std::endl;

	return faces.size();

	/* Drawing ellipse, circlbRecogEnabledes
	 *
	 for(int i = 0; i < faces.size(); i++){

	 Point centerdetectFace( faces[i].x + faces[i].width*0.5, faces[i].y + faces[i].height*0.5 );

	 ellipse( rawImage, center, Size( faces[i].width*0.5, faces[i].height*0.5), 0, 0, 360, Scalar( 255, 0, 255 ), 4, 8, 0 );

	 Mat faceROI = frame_gray( faces[i] );

	 vector<Rect> eyes;

	 //-- In each face, detect eyes
	 eyes_cascade.detectMultiScale( faceROI, eyes, 1.1, 2, 0 |CV_HAAR_SCALE_IMAGE, Size(30, 30) );

	 for( int j = 0; j < eyes.size(); j++ )
	 {
	  	Point center( faces[i].x + eyes[j].x + eyes[j].width*0.5, faces[i].y + eyes[j].y + eyes[j].height*0.5 );
	  	int radius = cvRound( (eyes[j].width + eyes[i].height)*0.25 );
	  	circle( rawImage, center, radius, Scalar( 255, 0, 0 ), 4, 8, 0 );
	  }

	 }

	 */
}


bool ImageProcessing::validateResult(int prediction){

	//shall we filter this one

	//one map is needed <personID, personName>
	//prediction --> personName

	//check if personName is substr of contentFilter->params

	//std::cout << "validate results" <<std::endl;

	std::map<int, std::string>::iterator it = labelMap.find(prediction);

	if (it == labelMap.end()){
		std::cout << "Unknown" << std::endl;
		return false;
	}

	return ruleManager->isIncludedInNameList((*it).second);

}


ENUM_ACTION_TYPE ImageProcessing::filteredIn(int frameCount){

	ENUM_ACTION_TYPE action;

	//1. check the filterdInFrameList of each rule
	action = ruleManager->getActionForFrame(frameCount);

	//std::cout << "action is " << action << std::endl;

	//if the results returned from time and location filtering is no_action
	//, then we check face
	//blur the whole image and deletion gets higher priority
	if (action < REMOVE_FACE){

		if (ruleManager->isFaceDetectionEnabled() || ruleManager->isFaceRecogEnabled())
			action = REMOVE_FACE;

	} else if (action == BLUR_IMAGE){

		//when shall we remove all the faces?
		if (ruleManager->isFaceDetectionEnabled() || ruleManager->isFaceRecogEnabled())
			action = BLUR_AND_REMOVE_FACE;
	}

	return action;

}


void ImageProcessing::loadConfiguration(){

	json_error_t err_t;
	json_t* root = json_load_file("XMLFiles/config.json", 0, &err_t);
	json_t* dmAddr = NULL;
	json_t* dmPort = NULL;
	json_t* width = NULL;
	json_t* height = NULL;
	json_t* recognizer = NULL;
	json_t* area = NULL;
	json_t* detector = NULL;
	json_t* rate = NULL;
	json_t* processingRate = NULL;

	if (root != NULL){

		dmAddr = json_object_get(root, "DATAMANAGER_URI");

		dmPort = json_object_get(root, "DATAMANAGER_PORT");

		width = json_object_get(root, "TRAINED_IMAGE_WIDTH");

		height = json_object_get(root, "TRAINED_IMAGE_HEIGHT");

		recognizer = json_object_get(root, "FACE_RECOGNIZER");

		area = json_object_get(root, "MIN_AREA");

		detector = json_object_get(root, "FRONTAL_FACE_DETECTOR");

		rate = json_object_get(root, "DETECTION_SAMPLE_RATE");

		processingRate = json_object_get(root, "DECODING_SAMPLE_RATE");
	}

	if (dmAddr != NULL)
		host_ = json_string_value(dmAddr);
	else
		host_ = "127.0.0.1";

	if (dmPort != NULL)
		port_ = atoi(json_string_value(dmPort));
	else
		port_ = 5000;

	if ((width != NULL) && (json_is_integer(width)))
		trainedImageWidth = json_integer_value(width);
	else
		trainedImageWidth = 2592;

	if ((height != NULL) && (json_is_integer(height)))
			trainedImageHeight = json_integer_value(height);
		else
			trainedImageHeight = 1936;


	if ((recognizer != NULL) && (json_is_string(recognizer))){

		std::string rec = json_string_value(recognizer);

		if (boost::iequals(rec, "LBPH"))
			recognizer_id = LBPH;
		else if (boost::iequals(rec, "FISHER"))
			recognizer_id = FISHER;
		else if (boost::iequals(rec, "EIGEN"))
			recognizer_id = EIGEN;
	}

	if ((area != NULL) && (json_is_integer(area))){

		min_area = json_integer_value(area);
	}


	if ((detector != NULL) && (json_is_string(detector))){

		if (boost::iequals(json_string_value(detector), "HAAR"))
			frontal_detector = FRONTAL_HAAR;
		else
			frontal_detector = FRONTAL_LBP;

	}

	json_t* neighbour = json_object_get(root, "MIN_NEIGHBOURS");
	if (neighbour != NULL)
		min_neighbour = json_integer_value(neighbour);
	else
		min_neighbour = 2;


	json_t* flag = json_object_get(root, "DETECTOR_FLAGS");
	if (flag != NULL)
		flags = json_integer_value(flag);
	else
		flags = 0;

	if (rate != NULL)
		sample_rate = json_integer_value(rate);
	else
		sample_rate = 1;

	if (processingRate != NULL)
		decoding_sample_rate = json_integer_value(processingRate);
	else
		decoding_sample_rate = 1;

	sample_rate *= decoding_sample_rate;

	json_object_clear(dmAddr);
	json_object_clear(dmPort);
	json_object_clear(rate);
	json_object_clear(root);

	std::cout << "trained image width: " << trainedImageWidth << "height " << trainedImageHeight << std::endl;
	std::cout << "min area width" << min_area << std::endl;
}

void ImageProcessing::setScale(const int i){
	scale = i;
}


