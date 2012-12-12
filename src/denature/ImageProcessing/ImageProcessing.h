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
#include <iostream>
#include <fstream>
#include <openssl/evp.h>

using namespace cv;

class ImageProcessing {
public:
	  enum TYPE_RECOGNIZER{
	    	LBPH = 0,
	    	FISHER = LBPH +1,
	    	EIGEN = FISHER+1
	    };

	 enum VIDEO_RESOLUTION{
		 LOW_320P,
		 MEDIUM_720P,
		 HIGH_1080P,
		 HIGH_1920P,
		 NOT_SUPPORTED
	 };

	 enum FRONTAL_FACE_DETECTOR{
		 FRONTAL_HAAR,
		 FRONTAL_LBP
	 };

	ImageProcessing(SharedQueue<std::pair<int,cv::Mat> >* readQueue, SharedQueue<std::pair<int, cv::Mat> >* writeQueue, ProcessingRuleContainer* rules, SharedQueue<SegmentInformation*>* segmentList);

	ImageProcessing(SharedQueue< std::pair<int, cv::Mat> >* readQueue);//for perf testing only
	virtual ~ImageProcessing();

	void operator()();
	void stop();
	
	void loadTrainingSet(const string& folderName, vector<Mat>& images, vector<int>& labels);
	bool trainRecognizer(TYPE_RECOGNIZER type);
	void perfTestDetection();
	void setScale(const int i);
	//void encryptVideo();

private:
	cv::String face_cascade_name;//frontal face
	cv::String profile_cascade_name;//profile face
    cv::CascadeClassifier face_cascade;
    cv::CascadeClassifier profile_cascade;
    Ptr<FaceRecognizer> model;
    SharedQueue<std::pair<int, cv::Mat> >* inputQueue;//where to get rawImage
    SharedQueue<std::pair<int, cv::Mat> >* outputQueue;//where to put processed images
    ProcessingRuleContainer* ruleManager;
    SharedQueue<SegmentInformation*>* segmentQueue;
    bool bEnding;
    std::map<int, std::string> labelMap;
    SegmentInformation* currentSegment;
    static const int MAX_KERNEL_LENGTH = 31;
    std::string host_;
    unsigned short port_;
    int trainedImageWidth;
    int trainedImageHeight;
    int min_area;

    TYPE_RECOGNIZER recognizer_id;
    bool bRecogEnabled;
    bool bLocEnabled;
    bool bTimeEnabled;
    VIDEO_RESOLUTION resolution_id;


    bool validateResult(int prediction);
    bool initFaceDetection();

    Mat& processFaces(Mat& rawImage, bool bBlur);
    int detectFace(Mat& rawImage);
    int recognizeFace(Mat& rawImage, vector<Rect>& faces);
    bool recognizeFace(Mat& face);

    ENUM_ACTION_TYPE filteredIn(int frameCount);//location, time-based filters
    bool preprocessing();
    void loadConfiguration();

    bool loadFaceRecognitionModels(std::string folderPath);
    int recognizeFaces(Mat& rawImage, int scale, int frameIndex);//if scale = 2, means we scale down the image width and height both to half of the original values
    int scale;
    FRONTAL_FACE_DETECTOR frontal_detector;
    int min_neighbour;
    int flags;
    //int AES_BLOCK_SIZE;
    //int aes_init(unsigned char *key_data, int key_data_len, unsigned char *salt, EVP_CIPHER_CTX *e_ctx, EVP_CIPHER_CTX *d_ctx);
    //unsigned char* aes_encrypt(EVP_CIPHER_CTX *e, unsigned char *plaintext, int *len);
   // unsigned char* aes_decrypt(EVP_CIPHER_CTX *e, unsigned char *ciphertext, int *len);
    int sample_rate;
    int decoding_sample_rate;
};

#endif /* IMAGEPROCESSING_H_ */
