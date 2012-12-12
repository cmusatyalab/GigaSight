/*
 * ProcessingRuleContainer.cpp
 *
 *  Created on: Oct 4, 2012
 *      Author: yuxiao
 *      In current implementation, we assume that the privacy setting is segment-specific. It is defined by mobile.
 *      If the privateVM receives a new privacy setting from the mobile, it will remove all the old rules (resetAllRules) and add only the new ones.
 */

#include "ProcessingRuleContainer.h"

ProcessingRuleContainer::ProcessingRuleContainer() {
	// TODO Auto-generated constructor stub

	bFaceDetectionEnabled = false;
	bFaceRecogEnabled = false;
	bLocationScoped = false;
}

ProcessingRuleContainer::~ProcessingRuleContainer() {
	// TODO Auto-generated destructor stub
}

void ProcessingRuleContainer::addRuleFromXMLFile(std::string& xmlFilePath){

	//load XML file

	//parse xml file

	//create a new processing rule
	PROCESSING_RULE rule;

	//add filters to rule.filterlist

	//get action type
	rule.actionType = REMOVE_FACE;


}


void ProcessingRuleContainer::addRule(PROCESSING_RULE& rule){

	ruleList.push_back(rule);

	if (!rule.locationFilterList.empty())
		bLocationScoped = true;

	if (!rule.faceFilterList.empty()){
		bFaceRecogEnabled = true;
	}

	if (rule.actionType == REMOVE_FACE)
		bFaceDetectionEnabled = true;
}


bool ProcessingRuleContainer::resetAllRules(){

	std::list<PROCESSING_RULE>::iterator list_it;

	for(list_it = ruleList.begin(); list_it!= ruleList.end(); list_it++)
	{
			ruleList.erase(list_it);
	}

	bFaceDetectionEnabled = false;
	bFaceRecogEnabled = false;
	bLocationScoped = false;



	return ruleList.empty();

	//we can consider reload the default settings
}


bool ProcessingRuleContainer::isFaceDetectionEnabled(){

	return bFaceDetectionEnabled;

}


bool ProcessingRuleContainer::isFaceRecogEnabled(){

	return bFaceRecogEnabled;

}


bool ProcessingRuleContainer::isLocationScopeEnabled(){

	return bLocationScoped;

}
