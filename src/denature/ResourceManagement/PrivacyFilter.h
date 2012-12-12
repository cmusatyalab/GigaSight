/*
 * PrivacyFilter.h
 *
 *  Created on: Oct 17, 2012
 *      Author: yuxiao
 */

#ifndef PRIVACYFILTER_H_
#define PRIVACYFILTER_H_
#include <list>

enum ENUM_FILTER_TYPE {
	LOCATION = 0,
	TIMEFRAME = LOCATION+1,
	CONTENT = TIMEFRAME+1
};

template <typename Data>

class PrivacyFilter {
private:
	std::list<Data> paramList;
	ENUM_FILTER_TYPE type;
	bool bFilteredIn;
	std::string description;

public:

	void addParam(const Data& p){
		paramList.push_back(p);
	};

	Data& getFirstParam(){

		if (paramList.begin() != paramList.end())
			return paramList.front();
		else
			return NULL;
	}

	std::list<Data>& getParamList(){

		return paramList;

	}

	void addDescription(std::string& s){
		description = s;
	};

	void setType(ENUM_FILTER_TYPE t){
		type = t;
	}

	ENUM_FILTER_TYPE& getType(){
		return type;
	}

	void setFilterInOrOut(bool b){
		bFilteredIn = b;
	}
};

#endif /* PRIVACYFILTER_H_ */
