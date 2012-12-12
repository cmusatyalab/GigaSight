/*
 * GPSUpload.h
 *
 *  Created on: Oct 12, 2012
 *      Author: Pieter Simoens
 */

#ifndef GPSUPLOAD_H_
#define GPSUPLOAD_H_

#include "UploadThread.h"

namespace upload {

class GPSUpload : public UploadThread {
public:
	GPSUpload() : UploadThread() {}


	void finishing(const std::string s);
	void stop();
};

} /* namespace upload */
#endif /* GPSUPLOAD_H_ */
