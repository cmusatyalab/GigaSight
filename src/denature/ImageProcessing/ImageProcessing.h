/*
 * ImageProcessing.h
 *
 *  Created on: Oct 4, 2012
 *      Author: yuxiao
 */

#ifndef IMAGEPROCESSING_H_
#define IMAGEPROCESSING_H_
#include "SharedQueue.h"
#include "VideoDecoding.h"
#include "../ResourceManagement/ProcessingRuleContainer.h"

using namespace cv;

class ImageProcessing {
public:
	ImageProcessing(SharedQueue<cv::Mat>* readQueue, SharedQueue<cv::Mat>* writeQueue, ProcessingRuleContainer* rules);
	virtual ~ImageProcessing();

	void operator()();
	void stop();
	
	void loadTrainingSet(const string& folderName, vector<Mat>& images, vector<int>& labels);


private:
	cv::String face_cascade_name;
	cv::String eyes_cascade_name;
    cv::CascadeClassifier face_cascade;
    cv::CascadeClassifier eyes_cascade;
    Ptr<FaceRecognizer> model;
    SharedQueue<cv::Mat>* inputQueue;//where to get rawImage
    SharedQueue<cv::Mat>* outputQueue;//where to put processed images
    ProcessingRuleContainer* ruleManager;
    bool bEnding;

    enum TYPE_RECOGNIZER{
    	LBPH = 0,
    	FISHER = LBPH +1,
    	EIGEN = FISHER+1
    };

    TYPE_RECOGNIZER recognizer_id;
    bool bRecogEnabled;

    bool validateResult(int prediction);
    bool initFaceDetection();

    Mat& processFaces(Mat& rawImage);
    vector<Rect>& detectFace(Mat& rawImage, vector<Rect>& faces);
    bool recognizeFace(Mat& face);
    bool trainRecognizer();
    bool filteredIn(int frameCount);//location, time-based filters
};

#endif /* IMAGEPROCESSING_H_ */
