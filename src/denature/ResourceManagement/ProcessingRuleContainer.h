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

typedef PrivacyFilter<double> LOCATION_FILTER;
typedef PrivacyFilter<std::string> FACE_FILTER;

enum ENUM_ACTION_TYPE {
		REMOVE_FACE = 0,
		BLUR_IMAGE = REMOVE_FACE+1,
		DELETE_FRAME = BLUR_IMAGE+1
};


struct PROCESSING_RULE{
	std::list<LOCATION_FILTER> locationFilterList;
	std::list<FACE_FILTER> faceFilterList;
	ENUM_ACTION_TYPE actionType;
	bool bActive;
};

class ProcessingRuleContainer {
private:
	std::list<PROCESSING_RULE> ruleList;
	bool bFaceDetectionEnabled;
	bool bFaceRecogEnabled;
	bool bLocationScoped;

public:
	ProcessingRuleContainer();
	virtual ~ProcessingRuleContainer();
	void addRuleFromXMLFile(std::string& xmlFilePath);
	void addRule(PROCESSING_RULE& rule);
	bool resetAllRules();
	bool isFaceDetectionEnabled();
	bool isFaceRecogEnabled();
	bool isLocationScopeEnabled();
};

#endif /* PROCESSINGRULECONTAINER_H_ */
