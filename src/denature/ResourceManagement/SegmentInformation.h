/*
 * SegmentInformation.h
 *
 *  Created on: Oct 17, 2012
 *      Author: yuxiao
 */

#ifndef SEGMENTINFORMATION_H_
#define SEGMENTINFORMATION_H_
#include "StreamInformation.h"

class SegmentInformation {
public:
	SegmentInformation();
	virtual ~SegmentInformation();
	void addToList(StreamInformation* newStream);
	StreamInformation* getByOutputType(STREAM_OUTPUT_TYPE type);
	bool removeFromList(const std::string id);
	StreamInformation* getByID(const std::string id);
	void setSegmentID(const std::string s);
	void reset();
	void setSourceType(STREAM_SOURCE_TYPE t);

private:
	std::string id;
	std::list<StreamInformation*> streamList;
	STREAM_SOURCE_TYPE sourceType;
};

#endif /* SEGMENTINFORMATION_H_ */
