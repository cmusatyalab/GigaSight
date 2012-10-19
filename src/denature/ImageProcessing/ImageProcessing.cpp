/*
 * ImageProcessing.cpp
 *
 *  Created on: Oct 4, 2012
 *      Author: yuxiao
 *  refer to http://www.opencv.org.cn/opencvdoc/2.3.1/html/doc/tutorials/objdetect/cascade_classifier/cascade_classifier.html#cascade-classifier.
 */

#include "ImageProcessing.h"
#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>


using namespace cv;
using namespace std;


ImageProcessing::ImageProcessing(SharedQueue<cv::Mat>* readQueue, SharedQueue<cv::Mat>* writeQueue, ProcessingRuleContainer* rules){

	inputQueue = readQueue;
	outputQueue = writeQueue;
	ruleManager = rules;

	recognizer_id = FISHER;
	bRecogEnabled = false; //use face recognition or not
	bEnding = false;
}


ImageProcessing::~ImageProcessing() {
	// TODO Auto-generated destructor stub
}


//when the thread starts, it first run operator()()
void ImageProcessing::operator()(){

	int frameCount = 0;


	if (!initFaceDetection()){
		cout << "Cascade classifiers cannot be initialized" << endl;
		return;
	}

	if (!trainRecognizer()){
		cout << "Recognizer cannot be initialized" << endl;
		return;
	}

	Mat rawImage;
	inputQueue->wait_and_pop(rawImage);

	//std::cout << "input" << inputQueue->size() << std::endl;



	while (!bEnding){


		if (!rawImage.empty()){

			++frameCount;

			if (ruleManager->isLocationScopeEnabled() && (!filteredIn(frameCount)))
				continue;

			outputQueue->push(processFaces(rawImage));

			//std::cout << "outputQueue size" << outputQueue->size() << std::endl;
		}
		else {
			//push an empty image to the queue to inform decodingthread that a new stream starts
			outputQueue->push(rawImage);
			std::cout<<frameCount + " frames processed"<<std::endl;
			frameCount = 0;
		}
	
		inputQueue->wait_and_pop(rawImage);

	}

}

void ImageProcessing::stop(){
	bEnding = true;
}

//Load classifiers
bool ImageProcessing::initFaceDetection(){
    
	std::cout << "initFaceDetection" <<std::endl;


    face_cascade_name = "XMLFiles/haarcascade_frontalface_alt.xml";
    eyes_cascade_name = "XMLFiles/haarcascade_eye_tree_eyeglasses.xml";
    
   //load a .xml classifier file. It can be either a Haar or a LBP classifier.

    if(!face_cascade.load(face_cascade_name)){
        cout << "Failed to load "<<face_cascade_name<< endl;
        return false;
    }
    
    if(!eyes_cascade.load(eyes_cascade_name)){
        cout << "Failed to load "<<eyes_cascade_name<< endl;
        return false;
    }

	return true;
}



Mat& ImageProcessing::processFaces(Mat& rawImage){

	vector<Rect> faces;
	Mat frame_gray;
	
	//convert the original image to gray scale
	cvtColor(rawImage, frame_gray, CV_BGR2GRAY);
	
	//Apply Histogram Equalization which improves the contrast in an image.
	equalizeHist(frame_gray, frame_gray);

	//-- Detect faces
	/*void CascadeClassifier::detectMultiScale(const Mat& image, vector<Rect>& objects, 
	*  double scaleFactor=1.1, int minNeighbors=3, int flags=0, Size minSize=Size())
	*/
  	face_cascade.detectMultiScale( frame_gray, faces, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE, Size(30, 30) );


  	std::cout << faces.size() << " faces detected" << std::endl;

  	if (faces.size() == 0)
  		return rawImage;


  	if (!ruleManager->isFaceRecogEnabled()){

  		//if we are going to remove all the faces from the photos,  no need to run face recognition
  		for(int i = 0; i < faces.size(); i++){
  			Rect face_i = faces[i];

  			//draw a green rectangle around the detected face
  			rectangle(rawImage, face_i, CV_RGB(0, 255,0), CV_FILLED);
  		}

  		return rawImage;
  	}


  	//Conduct face recognition

  	int im_width = rawImage.cols;
  	int im_height = rawImage.rows;

  	for(int i = 0; i < faces.size(); i++){

  		Rect face_i = faces[i];
  		int prediction;


  		if (recognizer_id != LBPH){
  			Mat face_resized;
  			// Crop the face from the image. So simple with OpenCV C++:

  			Mat face = frame_gray(face_i);


  			resize(face, face_resized, Size(im_width, im_height), 1.0, 1.0, INTER_CUBIC);
  		    prediction = model->predict(face_resized);
  		}
  		else
  			 prediction = model->predict(frame_gray);


  		std::cout << "prediction" << prediction << std::endl;

  		//prediction = the id of a person in training data set.
  		//are we going to remove the faces of all the persons included in the training set
  		if (!validateResult(prediction))
  			continue;

  		//if the face is recognized, do something here

  		// First of all draw a green rectangle around the detected face:
  		rectangle(rawImage, face_i, CV_RGB(0, 255,0), CV_FILLED);

  		// Create the text we will annotate the box with:
  		//string box_text = format("Prediction = %d", prediction);

  		// Calculate the position for annotated text (make sure we don't
  		// put illegal values in there):
  		//int pos_x = std::max(face_i.tl().x - 10, 0);
  		//int pos_y = std::max(face_i.tl().y - 10, 0);

  		// And now put it into the image:
  		//putText(rawImage, box_text, Point(pos_x, pos_y), FONT_HERSHEY_PLAIN, 1.0, CV_RGB(0,255,0), 2.0);

  		//imshow("face detection", rawImage );


  		}


  	return rawImage;

}

void ImageProcessing::loadTrainingSet(const string& folderName, vector<Mat>& images, vector<int>& labels){

	boost::filesystem3::path p(folderName);

	boost::filesystem3::directory_iterator end_iter;

	std::multimap<std::time_t, boost::filesystem3::path> result_set;

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

bool ImageProcessing::trainRecognizer(){
	// These vectors hold the images and corresponding labels:
    vector<Mat> images;
    vector<int> labels;
    
    //the .csv file should be created beforehand
    /*string cvsFileName = "training.csv";
    
    // Read in the data (fails if no valid input filename is given, but you'll get an error message):
    try {
        read_csv(cvsFileName, images, labels);
    } catch (cv::Exception& e) {
    	cerr << "Error opening file \"" << cvsFileName << "\". Reason: " << e.msg << endl;
        return false;
    }*/

    loadTrainingSet("TestFiles/", images, labels);


    if(images.size() <= 1) {
         std::cout<< "This demo needs at least 2 images to work. Please add more images to your data set!"<<std::endl;
         return false;
    }


    switch (recognizer_id){
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
    	model->train(images, labels);
    	std::cout << "trained fisher" << std::endl;
    }

    return true;
}


//if we want to run only detectFaces, use this method
vector<Rect>& ImageProcessing::detectFace(Mat& rawImage, vector<Rect>& faces){

	Mat frame_gray;

	//convert the original image to gray scale
	cvtColor(rawImage, frame_gray, CV_BGR2GRAY);

	//Apply Histogram Equalization which improves the contrast in an image.
	equalizeHist(frame_gray, frame_gray);

	//-- Detect faces
	/*void CascadeClassifier::detectMultiScale(const Mat& image, vector<Rect>& objects,
	*  double scaleFactor=1.1, int minNeighbors=3, int flags=0, Size minSize=Size())
	*/
	face_cascade.detectMultiScale( frame_gray, faces, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE, Size(30, 30) );


	std::cout << faces.size() << " faces detected" << std::endl;

	return faces;

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


	return true;
}


bool ImageProcessing::filteredIn(int frameCount){

	//use fps and frameCount to calculate the timestamp

	//time-based filter



	//timestamp can be used for synchronization with GPS traces

	//location-based filters

	//this means GPS stream needs to be buffered at privateVM until it has been processed


	return true;
}
