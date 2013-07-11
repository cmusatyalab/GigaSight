/*
 * ProcessingRuleContainer.cpp
 *
 *  Created on: Oct 4, 2012
 *      Author: yuxiao
 *      In current implementation, we assume that the privacy setting is segment-specific. It is defined by mobile.
 *      If the privateVM receives a new privacy setting from the mobile, it will remove all the old rules (resetAllRules) and add only the new ones.
 */

#include "ProcessingRuleContainer.h"
#include <iostream>
#include <math.h>
#include <iostream>
#include <fstream>
#include <vector>
//#include "boost/date_time/posix_time/posix_time_types.hpp"


ProcessingRuleContainer::ProcessingRuleContainer() {
	// TODO Auto-generated constructor stub

	bFaceDetectionEnabled = false;
	bFaceRecogEnabled = false;
	bLocationScoped = false;
	bTimeScoped = false;
	filteredInNames = "";
	max_size = 32;// max number of frameranges in the filteredFrameList[]
	ruleCount = 0;
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

	for (int i=0; i<max_size; i++)
		rule.filteredInFrameList[i] = std::pair<int, int>(0,0);

	ruleList.push_back(rule);

	if (!rule.locationFilterList.empty())
		bLocationScoped = true;

	if (!rule.faceFilterList.empty()){
		bFaceRecogEnabled = true;
	}

	if (!rule.timeFilterList.empty())
		bTimeScoped = true;

	if (rule.actionType == REMOVE_FACE)
		bFaceDetectionEnabled = true;

	ruleCount++;
}


bool ProcessingRuleContainer::resetAllRules(){

	while (ruleList.size() > 0)
		ruleList.pop_front();


	bFaceDetectionEnabled = false;
	bFaceRecogEnabled = false;
	bLocationScoped = false;

	filteredInNames = "";

	ruleCount = 0;

	std::cout << "RESET RULES" << std::endl;

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

bool ProcessingRuleContainer::isTimeScopeEnabled(){

	return bTimeScoped;

}

void ProcessingRuleContainer::addToNameList(const char* name){

	filteredInNames.append(name);
	filteredInNames.append(";");

}

std::string ProcessingRuleContainer::getNames(){

	return filteredInNames;
}

//this method has not been tested
bool ProcessingRuleContainer::isIncludedInNameList(const std::string name){

	if (!filteredInNames.empty()){

		std::string key = name+";";
		if (filteredInNames.find(key) != std::string::npos){
				return true;
		}
		else
				return false;
	}


	for (std::list<PROCESSING_RULE>::iterator i= ruleList.begin(); i != ruleList.end(); i++){
		std::list<FACE_FILTER> faceList;
		std::list<std::string> paramList;

		if (!(*i).faceFilterList.empty()){

			faceList = (*i).faceFilterList;

			for (std::list<FACE_FILTER>::iterator j = faceList.begin(); j != faceList.end(); ++j){

				paramList = (*j).getParamList();



				for (std::list<std::string>::iterator k = paramList.begin(); k != paramList.end(); ++k){

					//std::cout << (*k) << name <<  std::endl;

					if ((*k).find(name) != std::string::npos){
						return true;
					}

				}

			}

		}

	}

	return false;

}

std::string ProcessingRuleContainer::parseGPSRange(std::string gpsFilePath){


	std::ifstream inputFile;

	inputFile.open(gpsFilePath.c_str());

	if (!inputFile.is_open())
		return "";

	std::string line;
	double minLat = 180.0;
	double minLong = 180.0;
	double maxLat = -180.0;
	double maxLong = -180.0;
	double lat, lon;


	while (std::getline(inputFile, line)){

		lat = boost::lexical_cast<double>(line.substr(line.find_first_of(";", 0)+1, (line.find_last_of(";", line.size())-line.find_first_of(";", 0)-1)).c_str());
		lon = boost::lexical_cast<double>(line.substr(line.find_last_of(";")+1, (line.size()-line.find_last_of(";"))).c_str());
		std::cout << "lat:" << lat << "lon: " << lon << std::endl;

		if (lat > maxLat)
			maxLat = lat;

		if (lat < minLat)
			minLat = lat;

		if (lon > maxLong)
			maxLong = lon;

		if (lon < minLong)
			minLong = lon;

	}


	return boost::lexical_cast<std::string>(minLat)+";"+ boost::lexical_cast<std::string>(minLong)+";" + boost::lexical_cast<std::string>(maxLat)+";"+boost::lexical_cast<std::string>(maxLong)+";";

}



void ProcessingRuleContainer::parseFrameRangeByGPS(std::string gpsFilePath, double fps){

	std::cout << "PARSE RANGE BY GPS" << std::endl;

	PROCESSING_RULE rule;
	int index = 0;//index of filterInFrameList[]
	int lineCount = 0;
	std::ifstream inputFile;
	inputFile.open(gpsFilePath.c_str());
	double radius, lat1, lon1, dist;
	std::string currentTime;
	std::string line;
	double start, current, hour, min, sec;
	std::vector<struct point> pointList;
	struct point newPoint;

	int frameCount;


	if (!inputFile.is_open())
		return;


	//load gps file
	while( std::getline(inputFile, line) ) {

			 std::cout << line << std::endl;

			 currentTime = line.substr(line.find_first_of("T")+1, line.find_last_of("-", line.find_first_of(";"))-line.find("T")-1);

			// std::cout << currentTime << std::endl;

			 hour = boost::lexical_cast<double>(currentTime.substr(0, 2).c_str());
			 min = boost::lexical_cast<double>(currentTime.substr(currentTime.find(":", 0)+1, 2).c_str());
			 sec = boost::lexical_cast<double>(currentTime.substr( currentTime.find_last_of(":")+1, currentTime.size()-currentTime.find_last_of(":")-1).c_str());

			 current = hour*3600+min*60+sec;


			 if (lineCount == 0){
				 start = current;
		     }

		     lineCount++;

		     newPoint.lat = boost::lexical_cast<double>(line.substr(line.find_first_of(";")+1, (line.find_last_of(";")-line.find_first_of(";")-1)).c_str());
			 newPoint.lon = boost::lexical_cast<double>(line.substr(line.find_last_of(";")+1, (line.size()-line.find_last_of(";")-1)).c_str());

			 newPoint.time = current - start;


			 std::cout << "new point " << newPoint.time << ":" << newPoint.lat << ": " << newPoint.lon << std::endl;
			 pointList.push_back(newPoint);
	}



	for (std::list<PROCESSING_RULE>::iterator i= ruleList.begin(); i != ruleList.end(); i++){

		rule = (*i);

		if (rule.locationFilterList.empty()){
			std::cout << "Locationfilter list is empty" << std::endl;
			continue;
		}

		std::list<LOCATION_FILTER>::iterator it;
		std::list<double>::iterator j;


		//currently the size of filteredInFrameList is set to 32
		index = rule.index;


		for (it = rule.locationFilterList.begin(); it != rule.locationFilterList.end(); it++){

			if ((*it).getParamList().size() == 3){

				j = (*it).getParamList().begin();
				radius = *j;
				j++;
				lat1 = *j;
				j++;
				lon1 = *j;

				std::cout << "rule set radius: " << radius << "lat: " << lat1 << "lon: " << lon1 << std::endl;


				bool bInside = false;

				for (std::vector<struct point>::iterator m = pointList.begin(); m!= pointList.end(); m++){


					 dist = calculateDistance(lat1, lon1, (*m).lat, (*m).lon);

					 std::cout << "distance " << dist << std::endl;


					 if (dist <= radius){


						 //included in the range
						 frameCount = (*m).time * fps;

						 std::cout << (*m).time << ", " << (*i).filteredInFrameList[index].second <<  std::endl;

						 if (!bInside){
							 	 if (index >0 )
							 		 index++;

								(*i).filteredInFrameList[index].first = frameCount;
								(*i).filteredInFrameList[index].second = frameCount;

								bInside = true;
						 } else {

								 (*i).filteredInFrameList[index].second = frameCount;
						 }
					  }//if radius
					 else{
						 bInside = false;
					 }
				}//for vector

				std::cout << "location scoped from " << (*i).filteredInFrameList[index].first << " to " << (*i).filteredInFrameList[index].second << std::endl;
				//check if the location is included. try to jump instead of reading each one

		}//if paramList.size() == 3
		else {
			j = (*it).getParamList().begin();

			double minLat = radius = *j;
			j++;
			double minLong = *j;
			j++;
			double maxLat = *j;
			j++;
			double maxLong = *j;

			std::cout << "rule set leftdown, rightup(lat,lon)" << minLat << "," << minLong << " " << maxLat << ","<< maxLong << std::endl;


			bool bInside = false;

			for (std::vector<struct point>::iterator m = pointList.begin(); m!= pointList.end(); m++){


					if (((*m).lat >= minLat) && ((*m).lat <= maxLat) && ((*m).lon >= minLong) && ((*m).lon <= maxLong)) {

						 //included in the range
						 frameCount = (*m).time*fps;

						 if (!bInside){
						 	 if (index >0 )
						 		 index++;

						 	 (*i).filteredInFrameList[index].first = frameCount;
						 	 (*i).filteredInFrameList[index].second = frameCount;

						 	 bInside = true;
						 } else
						 	 (*i).filteredInFrameList[index].second = frameCount;
					}
					else
						bInside = false;

					std::cout << "location scoped from " << (*i).filteredInFrameList[index].first << " to " << (*i).filteredInFrameList[index].second << std::endl;
				}//for vector

							//check if the location is included. try to jump instead of reading each one
			}//for filterList

		}//else


		if ((*i).filteredInFrameList[0].second > 0)
			(*i).index = index+1;

	}//for ruleList


}

//give startTime, duration and framerate of the video, return a list of <startting frame, ending frame>
void ProcessingRuleContainer::parseFrameRangeByTime(std::string startTime, double duration, double fps){

	//use fps and frameCount to calculate the timestamp

	std::cout << "PARSE FRAME BY TIME" << std::endl;

	PROCESSING_RULE rule;
	int index = 0;//index of filterInFrameList[]

	std::string currentTime = startTime.substr(startTime.find_first_of("T")+1, startTime.find_last_of("-")-startTime.find("T")-1);

	if (currentTime.empty())
		return;

	double hour = atof(currentTime.substr(0, 2).c_str());
	double min = boost::lexical_cast<double>(currentTime.substr(currentTime.find(":", 0)+1, 2).c_str());
	double start = hour*3600 + min*60;
	double end = start + duration;
	double refStart, refEnd;

	std::cout << "start, end " << start << ", " << end << "rulelist size" << ruleList.size() << std::endl;



	for (std::list<PROCESSING_RULE>::iterator i= ruleList.begin(); i != ruleList.end(); i++){

		rule = (*i);

		if (rule.timeFilterList.empty()){
			std::cout << "no filter found" << std::endl;
			continue;
		}

		index = rule.index;

		std::cout << "time filter size " << rule.timeFilterList.size() << std::endl;

		for (std::list<TIME_FILTER>::iterator j = rule.timeFilterList.begin(); j != rule.timeFilterList.end(); j++){

			std::cout << "param size " << (*j).getParamList().size() << std::endl;

			for (std::list<std::pair<timepoint, timepoint> >::iterator k = (*j).getParamList().begin(); k != (*j).getParamList().end(); k++){


				std::cout << (*k).first.hour << ":" << (*k).first.min << std::endl;
				std::cout << (*k).second.hour << ":" << (*k).second.min << std::endl;

				refStart = (*k).first.hour*3600 + (*k).first.min*60;
				refEnd = (*k).second.hour*3600 + (*k).second.min*60;

				std::cout << "refstart, refend" << refStart << "," << refEnd << std::endl;

				if ((start <= refStart) && (end <= refEnd) && (end >= refStart)){
					(*i).filteredInFrameList[index] = std::pair<int,int>((int)(refStart-start)*fps, (int)(end-start)*fps);
					index++;
				} else if ((start >= refStart) && (end <= refEnd)){

					(*i).filteredInFrameList[index] = std::pair<int,int>(1, (int)(end-start)*fps);
					index++;
					std::cout << "frame 1 to " << (int)(end-start)*fps << "scoped" << std::endl;

				} else if ((start <= refStart) && (end >= refEnd)){

					(*i).filteredInFrameList[index] = std::pair<int,int>((int)(refStart-start)*fps, (int)(refEnd-start)*fps);
					index++;
				} else if ((start >= refStart) && (end >= refEnd)){
					(*i).filteredInFrameList[index] = std::pair<int,int>(1, (int)(refEnd-start)*fps);
					index++;
				}

			}//for


		}//for filter list

		(*i).index = index;

	}//for rule list


}


double ProcessingRuleContainer::calculateDistance(double lat1, double lon1, double lat2, double lon2){

	//convert from degrees to radians
   double pi = 3.1415926 / 180;

   double a1 = lat1 * pi;
   double b1 = lon1 * pi;
   double a2 = lat2 * pi;
   double b2 = lon2 * pi;

   const double r_e = 6378.135; //radius of the earth in kilometers (at the equator)

   //note that the earth is not a perfect sphere, r is also as small as
   const double r_p = 6356.75; //km at the poles

   //find the earth's radius at the average latitude between the two locations
   double theta = (lat1 + lat2) / 2;

   // r = Math.sqrt(((r_e**2 * Math.cos(theta))**2 + (r_p**2 * Math.cos(theta))**2) / ((r_e * Math.cos(theta))**2 + (r_p * Math.cos(theta))**2))
   double r = pow(pow(r_e, 2)*cos(theta), 2) + pow(pow(r_p,2)*cos(theta), 2);
   r = r/ (pow(r_e*cos(theta), 2) + pow(r_p*cos(theta), 2));

   //do the calculation with radians as units
   double toAcos = cos(a1)*cos(b1)* cos(a2)* cos(b2) + cos(a1)*sin(b1)*cos(a2)*sin(b2) + sin(a1)*sin(a2);

   if ((toAcos >= -1) and (toAcos <= 1))
     return sqrt(r) * acos(toAcos)*1000; //Converts distance from kilometers to meters
   else
     return 0;


}


ENUM_ACTION_TYPE ProcessingRuleContainer::getActionForFrame(int i){

	std::list<PROCESSING_RULE>::iterator it;
	PROCESSING_RULE rule;
	ENUM_ACTION_TYPE action = ACTION_NOT_SPECIFIED;

	//std::cout << "rule list size " << ruleList.size() << std::endl;

	for (it = ruleList.begin(); it != ruleList.end(); it++){

		rule = (*it);

		if (rule.index == 0){
			//std::cout << "rule index is 0"  << std::endl;
			continue;
		}

		for (int j = 0; j< rule.index; j++){

			//std::cout << "range: " << rule.filteredInFrameList[j].first << "to " << rule.filteredInFrameList[j].second << std::endl;


			if ((rule.filteredInFrameList[j].first <= i) && (rule.filteredInFrameList[j].second >= i)){



					if (rule.actionType > action){
						if ((action == REMOVE_FACE) && (rule.actionType == BLUR_IMAGE))
							action = BLUR_AND_REMOVE_FACE;
						else {
							action = rule.actionType;

						}
					}


					if (action == DELETE_FRAME)
						return action;
			}

		}//for

	}

	return action;
}


int ProcessingRuleContainer::getRuleCount(){
	return ruleCount;
}


void ProcessingRuleContainer::setFilteredInNames(const std::string s){
	filteredInNames.assign(s);

}

void ProcessingRuleContainer::setFaceRecognition(bool b){
	bFaceDetectionEnabled = b;

}
