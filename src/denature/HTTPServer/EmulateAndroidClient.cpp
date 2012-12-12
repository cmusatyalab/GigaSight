/*
 * EmulateAndroidClient.cpp
 *
 *  Created on: Dec 5, 2012
 *      Author: yuxiao
 */

#include "EmulateAndroidClient.h"

#include "request.hpp"
//#include "../HTTPServer/Client.h"



EmulateAndroidClient::EmulateAndroidClient() {
	// TODO Auto-generated constructor stub
	host_.assign("128.2.213.18");
	port_ = 12345;
	segID = "";
	streamID = "";
	rounds = 1;
	user_id = "";

	uploadInterval = 60;


}

EmulateAndroidClient::~EmulateAndroidClient() {
	// TODO Auto-generated destructor stub
	host_.clear();
}

void EmulateAndroidClient::setIteration(int i){
	rounds = i;
}

void EmulateAndroidClient::operator()(){

	loadConfiguration();

	std::ofstream logFile;
	boost::posix_time::ptime mst1, mst2;
	boost::posix_time::time_duration msdiff;

	std::string logFileName;
	logFileName.assign("emulatorLog");
	logFile.open(logFileName.c_str(), std::ios::out | std::ios::app);

	sleep(10);

	std::cout << "rounds: " << rounds << std::endl;

	for (int i=0; i < rounds; i++){
		mst1 = boost::posix_time::microsec_clock::local_time();

		std::cout << "round" << i << std::endl;

		createSegReq();

		createStreamReq();

		updateFileInfo();


		mst2 = boost::posix_time::microsec_clock::local_time();
		msdiff = mst2-mst1;

		sleep(uploadInterval - msdiff.seconds());

		logFile <<  msdiff.total_microseconds() << ";" << "\r\n";


	}

	logFile.close();

	std::cout << "file upload finishes!!!"  << std::endl;
}


bool EmulateAndroidClient::createSegReq(){


	boost::asio::streambuf request_;
	boost::asio::streambuf response_;

	//std::cout << "host_" << host_ << std::endl;
	//resolve the host name
	tcp::resolver::query query(host_, "http");

	tcp::endpoint endpoint;

	boost::asio::io_service io_service_;

	tcp::socket socket_(io_service_);

	//std::cout << "create seg req" << std::endl;
	try{
			//currently, we assume only one address will be returned

			tcp::resolver resolver_(io_service_);

			endpoint= *(resolver_.resolve(query));

	}catch (boost::system::system_error& e){

			std::cout << "cannot resolve the host" << e.what() << std::endl;
			return false;
	}


	//reset port number. otherwise, it would be 80
	endpoint.port(port_);

	//std::cout << "connecting to data manager port:" << endpoint.port() << std::endl;

	try{
			socket_.connect(endpoint);
	}catch (boost::system::system_error& e){

			std::cout << e.what() << std::endl;
			return false;
	}


	//first, generate requests
		json_t* root = json_object();

		json_object_set_new(root, "user_id", json_string(user_id.c_str()));
		json_object_set_new(root, "type", json_string("recorded"));

		char* jsonStr = json_dumps(root, 0);


		// Form the request. We specify the "Connection: close" header so that the
		// server will close the socket after transmitting the response. This will
		// allow us to treat all data up until the EOF as the content.
		std::ostream request_stream(&request_);
		request_stream << "POST " << "/segment" << " HTTP/1.1\r\n";
		request_stream << "Host: " << host_ << ":" << port_ << "\r\n";
		request_stream << "Accept-Encoding: identity\r\n";
		request_stream << "Content-Length: "<< std::string(jsonStr).length()<< "\r\n";
		request_stream << "Content-type: application/json\r\n";
		//always remember to put "\r\n\r\n" at the end of headers
		request_stream << "Connection: close\r\n\r\n";

		request_stream << jsonStr << "\r\n\r\n";


		std::cout << "segment request sent to private vm"<< jsonStr << std::endl;

		try{
			boost::asio::write(socket_, request_);

			//std::cout << "written" << std::endl;

			boost::asio::read_until(socket_, response_, "\r\n");

		}catch (boost::system::system_error& e){

			std::cout << "error" << e.what() << std::endl;
			return false;
		}

		std::cout << "receive response from private VM" << std::endl;

		//check status line
		std::istream response_stream(&response_);
		std::string http_version;
		response_stream >> http_version;



		if (!response_stream || http_version.substr(0, 5) != "HTTP/"){

			std::cout << "Invalid response " << response_stream << std::endl;
			return false;

		}

		unsigned int status_code;
		response_stream >> status_code;

		std::cout << "Response returned with status code " << status_code << std::endl;


		std::ostringstream response;

		boost::system::error_code error;
		boost::asio::read(socket_, response_, boost::asio::transfer_all(), error);

		response << &response_;

		std::cout << "headers:\n"<< response.str() << std::endl;

		segID = response.str().substr(response.str().find("/"), response.str().find_last_of("/")-response.str().find("/")+1);


		std::cout << "parse segID is " << segID << std::endl;


		json_object_clear(root);
		socket_.close();
		response_stream.clear();


		return true;

}

bool EmulateAndroidClient::createStreamReq(){

	boost::asio::streambuf request_;
	boost::asio::streambuf response_;

	//resolve the host name
	tcp::resolver::query query(host_, "http");

	tcp::endpoint endpoint;
	boost::asio::io_service io_service_;
	tcp::socket socket_(io_service_);


	try{
			//currently, we assume only one address will be returned

			tcp::resolver resolver_(io_service_);

			endpoint= *(resolver_.resolve(query));

	}catch (boost::system::system_error& e){

			std::cout << "cannot resolve the host" << e.what() << std::endl;
			return false;
	}


	//reset port number. otherwise, it would be 80
	endpoint.port(port_);

	//std::cout << "connecting to data manager port:" << endpoint.port() << std::endl;

	try{
			socket_.connect(endpoint);
	}catch (boost::system::system_error& e){

			std::cout << e.what() << std::endl;
			return false;
	}


	//first, generate requests
		json_t* root = json_object();

		json_object_set_new(root, "container", json_string("MP4"));

		char* jsonStr = json_dumps(root, 0);


		// Form the request. We specify the "Connection: close" header so that the
		// server will close the socket after transmitting the response. This will
		// allow us to treat all data up until the EOF as the content.
		std::ostream request_stream(&request_);
		request_stream << "POST " << segID << " HTTP/1.1\r\n";
		request_stream << "Host: " << host_ << ":" << port_ << "\r\n";
		request_stream << "Accept-Encoding: identity\r\n";
		request_stream << "Content-Length: "<< std::string(jsonStr).length()<< "\r\n";
		request_stream << "Content-type: application/json\r\n";
		//always remember to put "\r\n\r\n" at the end of headers
		request_stream << "Connection: close\r\n\r\n";

		request_stream << jsonStr << "\r\n\r\n";

		std::cout << "segment request sent to private vm"<< jsonStr << std::endl;


		try{
			boost::asio::write(socket_, request_);

			boost::asio::read_until(socket_, response_, "\r\n");

		}catch (boost::system::system_error& e){

			std::cout << e.what() << std::endl;
			return false;
		}

		//check status line
		std::istream response_stream(&response_);
		std::string http_version;
		response_stream >> http_version;

		if (!response_stream || http_version.substr(0, 5) != "HTTP/"){

			std::cout << "Invalid response \n";
			return false;

		}

		unsigned int status_code;
		response_stream >> status_code;

		std::cout << "Response returned with status code " << status_code << std::endl;


		std::ostringstream response;

		boost::system::error_code error;
		boost::asio::read(socket_, response_, boost::asio::transfer_all(), error);

		response << &response_;

		std::cout << "headers:\n"<< response.str() << std::endl;

		streamID = response.str().substr(response.str().find("/"), response.str().find_last_of("/")-response.str().find("/")+1);


		std::cout << "mobile receives streamID " << streamID << std::endl;


		json_object_clear(root);
		socket_.close();
		response_stream.clear();

		return true;

}


bool EmulateAndroidClient::updateFileInfo(){

	std::cout << "update file info - mobile" << std::endl;
	boost::asio::streambuf request_;
	boost::asio::streambuf response_;

	//resolve the host name
	tcp::resolver::query query(host_, "http");

	tcp::endpoint endpoint;
	boost::asio::io_service io_service_;
	tcp::socket socket_(io_service_);


	try{
			//currently, we assume only one address will be returned

			tcp::resolver resolver_(io_service_);

			endpoint= *(resolver_.resolve(query));

	}catch (boost::system::system_error& e){

			std::cout << "cannot resolve the host" << e.what() << std::endl;
			return false;
	}


	//reset port number. otherwise, it would be 80
	endpoint.port(port_);

	//std::cout << "connecting to data manager port:" << endpoint.port() << std::endl;

	try{
			socket_.connect(endpoint);
	}catch (boost::system::system_error& e){

			std::cout << e.what() << std::endl;
			return false;
	}


	//first, generate requests
	json_t* root = json_object();

	json_object_set_new(root, "container", json_string("MP4"));
	json_object_set_new(root, "startTime", json_string("2012-12-04T15:30:00:000Z"));
	json_object_set_new(root, "duration", json_string("160000"));
	json_object_set_new(root, "resolution", json_string("1920x1080"));


	char* jsonStr = json_dumps(root, 0);


		// Form the request. We specify the "Connection: close" header so that the
		// server will close the socket after transmitting the response. This will
		// allow us to treat all data up until the EOF as the content.


		std::ostream request_stream(&request_);
		request_stream << "PUT " << streamID << " HTTP/1.1\r\n";
		request_stream << "Host: " << host_ << ":" << port_ << "\r\n";
		request_stream << "Accept-Encoding: identity\r\n";
		request_stream << "Content-Length: "<< std::string(jsonStr).length()<< "\r\n";
		request_stream << "Content-type: application/json\r\n";
		//always remember to put "\r\n\r\n" at the end of headers
		request_stream << "Connection: close\r\n\r\n";

		request_stream << jsonStr << "\r\n\r\n";


		std::cout << "segment request sent to private vm"<< jsonStr << std::endl;


		try{
			boost::asio::write(socket_, request_);

			boost::asio::read_until(socket_, response_, "\r\n");

		}catch (boost::system::system_error& e){

			std::cout << e.what() << std::endl;
			return false;
		}

		//check status line
		std::istream response_stream(&response_);
		std::string http_version;
		response_stream >> http_version;

		if (!response_stream || http_version.substr(0, 5) != "HTTP/"){

			std::cout << "Invalid response \n";
			return false;

		}


		json_object_clear(root);
		socket_.close();
		response_stream.clear();

		return true;

}


void EmulateAndroidClient::loadConfiguration(){

	json_error_t err_t;
	json_t* root = json_load_file("XMLFiles/config.json", 0, &err_t);
	json_t* phoneID = NULL;
	json_t* path = NULL;
	json_t* interval = NULL;
	json_t* pvAddr = NULL;
	json_t* pvPort = NULL;

	if (root != NULL){

		pvAddr = json_object_get(root, "PRIVATEVM_URI");
		pvPort = json_object_get(root, "PRIVATEVM_TCP_PORT");
		phoneID = json_object_get(root, "USER_ID");
		path = json_object_get(root, "TEST_FILES");
		interval = json_object_get(root, "UPLOAD_INTERVAL");

	}



	if (phoneID != NULL)
		user_id = json_string_value(phoneID);
	else
		user_id = "";

	if ((interval != NULL) && json_is_integer(interval))
		uploadInterval = json_integer_value(interval);
	else
		uploadInterval = 5;//seconds

	if (path != NULL)
		folder = json_string_value(path);
	else
		folder = "";


	boost::filesystem3::path filePath(folder);
	if (!boost::filesystem3::exists(filePath))
		rounds = 0;
	else{

			if (boost::filesystem3::is_directory(filePath)){

				rounds = 0;
				boost::filesystem3::directory_iterator end_iter;
				for( boost::filesystem3::directory_iterator dir_iter(filePath) ; dir_iter != end_iter ; ++dir_iter)
				{
					if (boost::filesystem3::is_regular_file((*dir_iter).path()))
						rounds++;
				}
			}
			else
				rounds = 1;
	}

	std::cout << "supposed to test " << rounds << "files" << std::endl;


	if ((pvAddr != NULL) && json_is_string(pvAddr))
		host_ = json_string_value(pvAddr);
	else
		host_ = "127.0.0.1";

	if ((pvPort != NULL) && json_is_string(pvPort))
		port_ = atoi(json_string_value(pvPort));
	else
		port_ = 12345;



	json_object_clear(path);
	json_object_clear(phoneID);
	json_object_clear(interval);
	json_object_clear(pvAddr);
	json_object_clear(pvPort);
	json_object_clear(root);

	std::cout << "private VM: " << host_ << ":" << port_ << std::endl;
}
