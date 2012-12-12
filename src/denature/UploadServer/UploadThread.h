/*
 * UploadThread.h
 *
 *  Created on: Oct 11, 2012
 *      Author: Pieter Simoens
 */

#ifndef UPLOADTHREAD_H_
#define UPLOADTHREAD_H_

#include "../ImageProcessing/SharedQueue.h"
#include "../ResourceManagement/SegmentInformation.h"

namespace upload {

class UploadThread {
public:
	UploadThread();
	virtual ~UploadThread();

	// Run the main loop of this manager.
	void start(int fd, SharedQueue<SegmentInformation*>* s);
	void startReceive();
	virtual void finishing(const std::string s) = 0;
	virtual void stop() = 0;
protected:
	int fd;
	bool keepRunning;
	SharedQueue<SegmentInformation*>* s_queue;
private:
	void cleanUp();

};

} /* namespace upload */
#endif /* UPLOADTHREAD_H_ */
