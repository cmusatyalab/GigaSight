/*
 * RTSPRTPUpload.h
 *
 *  Created on: Oct 12, 2012
 *      Author: pieter
 */

#ifndef RTSPRTPUPLOAD_H_
#define RTSPRTPUPLOAD_H_

#include "UploadThread.h"

namespace upload {

class RTSP_RTPUpload : public UploadThread {
public:
	RTSP_RTPUpload() : UploadThread() {}


	void finishing(const std::string s);
	void stop();
};

} /* namespace upload */
#endif /* RTSPRTPUPLOAD_H_ */
