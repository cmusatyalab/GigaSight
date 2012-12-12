/*
 * ProcessingRuleContainer.h
 *
 *  Created on: Oct 4, 2012
 *      Author: yuxiao
 */

#ifndef PROCESSINGRULECONTAINER_H_
#define PROCESSINGRULECONTAINER_H_

#include <string>
#include "PrivacyFilter.h"
#include "boost/date_time/posix_time/posix_time.hpp"

struct timepoint{
	int hour;
	int min;
	int second;
};

typedef PrivacyFilter<double> LOCATION_FILTER;
typedef PrivacyFilter<std::string> FACE_FILTER;
typedef PrivacyFilter<std::pair<timepoint, timepoint> > TIME_FILTER;


enum ENUM_ACTION_TYPE {
	ACTION_NOT_SPECIFIED = 0,
	NO_ACTION = ACTION_NOT_SPECIFIED +1,
	REMOVE_FACE = NO_ACTION +1,
	BLUR_IMAGE = REMOVE_FACE+1,
	BLUR_AND_REMOVE_FACE = BLUR_IMAGE+1,
	DELETE_FRAME = BLUR_AND_REMOVE_FACE+1
};


struct PROCESSING_RULE{
	std::list<LOCATION_FILTER> locationFilterList;
	std::list<FACE_FILTER> faceFilterList;
	std::list<TIME_FILTER> timeFilterList;
	ENUM_ACTION_TYPE actionType;
	bool bActive;
	std::pair<int, int> filteredInFrameList[32];
	int index;//index of filteredInFrameList
	bool bLocationByRadius;
};

struct point{
	double time;
	double lat;
	double lon;
};

class ProcessingRuleContainer {
private:
	std::list<PROCESSING_RULE> ruleList;
	bool bFaceDetectionEnabled;
	bool bFaceRecogEnabled;
	bool bLocationScoped;
	bool bTimeScoped;
	std::string filteredInNames;
	int max_size;
	int ruleCount;


public:
	ProcessingRuleContainer();
	virtual ~ProcessingRuleContainer();
	void addRuleFromXMLFile(std::string& xmlFilePath);
	void addRule(PROCESSING_RULE& rule);
	bool resetAllRules();
	bool isFaceDetectionEnabled();
	bool isFaceRecogEnabled();
	bool isLocationScopeEnabled();
	bool isIncludedInNameList(const std::string name);
	void addToNameList(const char* name);
	bool isTimeScopeEnabled();
	void parseFrameRangeByGPS(std::string gpsFilePath, double fps);
	void parseFrameRangeByTime(std::string startTime, double duration, double fps);
	double calculateDistance(double lat1, double lon1, double lat2, double lon2);
	ENUM_ACTION_TYPE getActionForFrame(int i);
	std::string getNames();
	std::string parseGPSRange(std::string gpsFilePath);
	int getRuleCount();
	void setFilteredInNames(const std::string s);
	void setFaceRecognition(bool b);
};

#endif /* PROCESSINGRULECONTAINER_H_ */
