/*
 * UploadManager.h
 *
 *  Created on: Oct 11, 2012
 *      Author: Pieter Simoens
 */

#ifndef UPLOADMANAGER_H_
#define UPLOADMANAGER_H_

#include <boost/thread.hpp>
#include "UploadThread.h"
#include "UploadProtocol.h"
#include "../ImageProcessing/SharedQueue.h"
#include "../ResourceManagement/SegmentInformation.h"

namespace upload {

class UploadManager {
public:
	static UploadManager& Instance(SharedQueue<SegmentInformation*>* s);

	/// Run the main loop of this manager.
	void run();
	// Stop the manager
	void stop();

	//port on which uploads are going to be received
	int getPort();

	static const int DEFAULTPORT = 5555;

	void notifyFinished(boost::thread::id id);

private:
	int listenPort;
	int listenFD;

	static void CleanUp();
	UploadManager(SharedQueue<SegmentInformation*>* s);
	~UploadManager() {}

	// not copyable
	UploadManager(UploadManager const&);
	UploadManager& operator=(UploadManager const&);

	static UploadManager* MInstance;
	bool keepRunning;

	SharedQueue<SegmentInformation*>* s_queue;

	void handleNewConnection(int);
};

} /* namespace upload */
#endif /* UPLOADMANAGER_H_ */
