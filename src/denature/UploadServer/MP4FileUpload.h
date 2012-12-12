/*
 * MP4FileUpload.h
 *
 *  Created on: Oct 12, 2012
 *      Author: Pieter Simoens
 */

#ifndef MP4FILEUPLOAD_H_
#define MP4FILEUPLOAD_H_

#include "UploadThread.h"
namespace upload {

class MP4FileUpload : public UploadThread {
public:
	MP4FileUpload() : UploadThread(){};


	void finishing(std::string s);
	void stop();
};

} /* namespace upload */
#endif /* MP4FILEUPLOAD_H_ */
