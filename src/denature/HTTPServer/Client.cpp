/*
 * Client.cpp
 *
 *  Created on: Oct 18, 2012
 *  boost async_client example
 */

#include "Client.h"



Client::~Client() {
	// TODO Auto-generated destructor stub
}

Client::Client() {

}

bool Client::requestForSegment(const std::string& host_, const unsigned short port_, const std::string& user_id, std::string& result){



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

	json_object_set_new(root, "user_id", json_string(user_id.c_str()));

	char* jsonStr = json_dumps(root, 0);



	// Form the request. We specify the "Connection: close" header so that the
	// server will close the socket after transmitting the response. This will
	// allow us to treat all data up until the EOF as the content.
	std::ostream request_stream(&request_);
	request_stream << "POST " << "/api/dm/segment/" << " HTTP/1.1\r\n";
	request_stream << "Host: " << host_ << ":" << port_ << "\r\n";
	request_stream << "Accept-Encoding: identity\r\n";
	request_stream << "Content-Length: "<< std::string(jsonStr).length()<< "\r\n";
	request_stream << "Content-type: application/json\r\n";
	//always remember to put "\r\n\r\n" at the end of headers
	request_stream << "Connection: close\r\n\r\n";

	request_stream << jsonStr << "\r\n\r\n";

	std::cout << "segment request sent to dm"<< jsonStr << std::endl;


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


	std::string body;

	std::cout << "headers:\n"<< response.str() << std::endl;

	size_t i = response.str().find("{");

	if (i != response.str().npos){
		body = response.str().substr(i, response.str().size());
	}

	std::cout << "body:\n" << body << std::endl;

	json_t* responseRoot = NULL;

	json_error_t err_t;
	json_t* filePath;

	responseRoot = json_loads(body.c_str(), 0, &err_t);

	if (responseRoot == NULL){

		std::cout << "cannot load the response body" << std::endl;

		return false;
	}


	filePath = json_object_get(responseRoot, "resource_uri");

	if (filePath != NULL){

	  	  if (!json_is_array(filePath)){

			  /*reply_.status = http::server::reply::created;
			  reply_.headers.resize(1);
			  reply_.headers[0].name = "Location";
			  reply_.headers[0].value = json_string_value(filePath);*/

			  //std::cout << "reply" << json_string_value(filePath) << std::endl;

			  result.assign(json_string_value(filePath));
	   	  }
	}

	return true;

}


//type: 0: original video  1: denatured video 2: GPS
bool Client::requestForStream(const std::string& host_, const unsigned short port_, const http::server::request& req, int type, std::string& uri, std::string& path){

	//new stream
	//std::string segID = req.uri.substr(9, req.uri.size()-9);
	std::string dmSegID = "/api/dm"+req.uri;
	std::cout << dmSegID << std::endl;
	json_t* request = json_object();

	json_object_set_new(request, "segment", json_string(dmSegID.c_str()));

	json_error_t err_t;
	json_t* root = json_loads(req.body.c_str(), 0, &err_t);

	if (root == NULL)
			return false;

	json_t* container = json_object_get(root, "container");;

	std::string description;

	if (container != NULL){
		description.assign(json_string_value(container));
		description.append(";");
	}
	else
		description = "";


	switch(type){
	case 0:
		//original
		//json_object_set_new(request, "tag", json_string("original"));
		description.append("video;original;public;");
		break;
	case 1:
		//json_object_set_new(request, "tag", json_string("denatured"));
		description.append("video;denatured;public;");
		break;
	case 2:
		//json_object_set_new(request, "tag", json_string("frames"));
		description.append("gps;original;public;");
		break;
	}

	json_object_set_new(request, "stream_description", json_string(description.c_str()));

	char* jsonStr = json_dumps(request, 0);


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

			std::cout << e.what() << std::endl;
			return false;
	}


	//reset port number. otherwise, it would be 80
	endpoint.port(port_);

	try{
			socket_.connect(endpoint);
	}catch (boost::system::system_error& e){

		std::cout << e.what() << std::endl;
		return false;
	}



	std::ostream request_stream(&request_);
	request_stream << "POST " << "/api/dm/stream/"<< " HTTP/1.1\r\n";
	request_stream << "Host: " << host_ << ":" << port_ << "\r\n";
	request_stream << "Accept-Encoding: identity\r\n";
	request_stream << "Content-Length: "<< std::string(jsonStr).length()<< "\r\n";
	request_stream << "Content-type: application/json\r\n";
	//always remember to put "\r\n\r\n" at the end of headers
	request_stream << "Connection: close\r\n\r\n";

	request_stream << jsonStr << "\r\n\r\n";

	std::cout << "sent to dm" << jsonStr;


	try{

		boost::asio::write(socket_, request_);

		boost::asio::read_until(socket_, response_, "\r\n");
	} catch(boost::system::system_error& e){

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


	//boost::asio::read_until(socket_, response_, "\r\n\r\n");

	//std::ostringstream response;
	//response << &response_;
	std::string headers, body;

	headers = response.str();
	//response_stream >> headers;

	std::cout << "headers:\n" << headers << std::endl;

	size_t i = headers.find("{");

	if (i != headers.npos){
		body = headers.substr(i, headers.size());
	}

	std::cout << "body:\n" << body << std::endl;

	json_t* responseRoot = NULL;

	json_t* uriNode;
	json_t* filePathNode;

	responseRoot = json_loads(body.c_str(), 0, &err_t);

	if (responseRoot == NULL){

			std::cout << "cannot load" << std::endl;

			return false;
	}


	filePathNode = json_object_get(responseRoot, "path");
	uriNode = json_object_get(responseRoot, "resource_uri");

	//resource_uri = /api/dm/stream/streamID/

	if (uriNode != NULL)
		uri.assign(json_string_value(uriNode));

	if (filePathNode != NULL)
		path.assign(json_string_value(filePathNode));

	return true;

}

void Client::updateProcessingStatus(const std::string& host_, const unsigned short port_, const std::string streamID, const PROCESSING_STATUS statusCode){

	//update stream information
	std::string url= "/api/dm/stream" +streamID.substr(streamID.find("/", 9), streamID.size());
	json_t* request = json_object();

	switch (statusCode){
	case INITIATED:
		json_object_set_new(request, "status", json_string("CRE"));
		break;
	case PROCESSING:
		json_object_set_new(request, "status", json_string("UPD"));
		break;
	case STOPPED:
		json_object_set_new(request, "status", json_string("FIN"));
		break;
	}


	char* jsonStr = json_dumps(request, 0);


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

		std::cout << e.what() << std::endl;
		return;
	}

	//reset port number. otherwise, it would be 80
	endpoint.port(port_);

	try{
			socket_.connect(endpoint);
	}catch (boost::system::system_error& e){

			std::cout << e.what() << std::endl;
			return;
	}


	std::ostream request_stream(&request_);
	request_stream << "PUT " << url << " HTTP/1.1\r\n";
	request_stream << "Host: " << host_ << ":" << port_ << "\r\n";
	request_stream << "Accept-Encoding: identity\r\n";
	request_stream << "Content-Length: "<< std::string(jsonStr).length()<< "\r\n";
	request_stream << "Content-type: application/json\r\n";
	//always remember to put "\r\n\r\n" at the end of headers
	request_stream << "Connection: close\r\n\r\n";

	request_stream << jsonStr << "\r\n\r\n";

	std::cout << "url" << url << "body" << jsonStr << std::endl;


	boost::asio::write(socket_, request_);

	boost::asio::read_until(socket_, response_, "\r\n");

	//check status line
	std::istream response_stream(&response_);
	std::string http_version;
	response_stream >> http_version;

	if (!response_stream || http_version.substr(0, 5) != "HTTP/"){

		std::cout << "Invalid response \n";
		return;

	}

	unsigned int status_code;
	response_stream >> status_code;

	std::cout << "Response returned with status code " << status_code << std::endl;
	boost::asio::read_until(socket_, response_, "\r\n\r\n");

}


void Client::updateStreamInformation(const std::string& host_, const unsigned short port_, const std::string streamID, std::string body){

	//update stream information
		std::string url= "/api/dm/stream" +streamID.substr(streamID.find("/", 9), streamID.size());

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

			std::cout << e.what() << std::endl;
			return;
		}

		//reset port number. otherwise, it would be 80
		endpoint.port(port_);

		try{
				socket_.connect(endpoint);
		}catch (boost::system::system_error& e){

				std::cout << e.what() << std::endl;
				return;
		}


		std::ostream request_stream(&request_);
		request_stream << "PUT " << url << " HTTP/1.1\r\n";
		request_stream << "Host: " << host_ << ":" << port_ << "\r\n";
		request_stream << "Accept-Encoding: identity\r\n";
		request_stream << "Content-Length: "<< body.length()<< "\r\n";
		request_stream << "Content-type: application/json\r\n";
		//always remember to put "\r\n\r\n" at the end of headers
		request_stream << "Connection: close\r\n\r\n";

		request_stream << body << "\r\n\r\n";

		//std::cout << "url" << url << "body" << body << std::endl;


		boost::asio::write(socket_, request_);

		boost::asio::read_until(socket_, response_, "\r\n");

		//check status line
		std::istream response_stream(&response_);
		std::string http_version;
		response_stream >> http_version;

		if (!response_stream || http_version.substr(0, 5) != "HTTP/"){

			std::cout << "Invalid response \n";
			return;

		}

		unsigned int status_code;
		response_stream >> status_code;

		//std::cout << "Response returned with status code " << status_code << std::endl;
		boost::asio::read_until(socket_, response_, "\r\n\r\n");

}


void Client::updateSegmentInformation(const std::string& host_, const unsigned short port_, const std::string segmentID, std::string body){

	//update stream information
		std::string url= "/api/dm" +segmentID;

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

			std::cout << e.what() << std::endl;
			return;
		}

		//reset port number. otherwise, it would be 80
		endpoint.port(port_);

		try{
				socket_.connect(endpoint);
		}catch (boost::system::system_error& e){

				std::cout << e.what() << std::endl;
				return;
		}


		std::ostream request_stream(&request_);
		request_stream << "PUT " << url << " HTTP/1.1\r\n";
		request_stream << "Host: " << host_ << ":" << port_ << "\r\n";
		request_stream << "Accept-Encoding: identity\r\n";
		request_stream << "Content-Length: "<< body.length()<< "\r\n";
		request_stream << "Content-type: application/json\r\n";
		//always remember to put "\r\n\r\n" at the end of headers
		request_stream << "Connection: close\r\n\r\n";

		request_stream << body << "\r\n\r\n";

		//std::cout << "url" << url << "body" << body << std::endl;


		boost::asio::write(socket_, request_);

		boost::asio::read_until(socket_, response_, "\r\n");

		//check status line
		std::istream response_stream(&response_);
		std::string http_version;
		response_stream >> http_version;

		if (!response_stream || http_version.substr(0, 5) != "HTTP/"){

			std::cout << "Invalid response \n";
			return;

		}

		unsigned int status_code;
		response_stream >> status_code;

		//std::cout << "Response returned with status code " << status_code << std::endl;
		boost::asio::read_until(socket_, response_, "\r\n\r\n");

}
/*
void Client::handle_resolve(const boost::system::error_code& err,
      tcp::resolver::iterator endpoint_iterator)
  {
    if (!err)
    {
      // Attempt a connection to the first endpoint in the list. Each endpoint
      // will be tried until we successfully establish a connection.
      tcp::endpoint endpoint = *endpoint_iterator;

      endpoint.port(port_);

      std::cout << endpoint.address() << endpoint.port() << std::endl;

      socket_.async_connect(endpoint,
    		boost::bind(&Client::handle_connect, this,
            boost::asio::placeholders::error, ++endpoint_iterator));
    }
    else
    {
      std::cout << "Error: " << err.message() << "\n";
    }
  }

void Client::handle_connect(const boost::system::error_code& err,
      tcp::resolver::iterator endpoint_iterator)
  {
    if (!err)
    {
      // The connection was successful. Send the request.
      boost::asio::async_write(socket_, request_,
          boost::bind(&Client::handle_write_request, this,
            boost::asio::placeholders::error));
    }
    else if (endpoint_iterator != tcp::resolver::iterator())
    {
      // The connection failed. Try the next endpoint in the list.
      socket_.close();
      tcp::endpoint endpoint = *endpoint_iterator;
      endpoint.port(port_);

      std::cout << endpoint.address() << endpoint.port() << std::endl;
      socket_.async_connect(endpoint,
          boost::bind(&Client::handle_connect, this,
            boost::asio::placeholders::error, ++endpoint_iterator));
    }
    else
    {
      std::cout << "Error: " << err.message() << "\n";
    }
  }

  void Client::handle_write_request(const boost::system::error_code& err)
  {
    if (!err)
    {
      // Read the response status line.
      boost::asio::async_read_until(socket_, response_, "\r\n",
          boost::bind(&Client::handle_read_status_line, this,
            boost::asio::placeholders::error));
    }
    else
    {
      std::cout << "Error: " << err.message() << "\n";
    }
  }



void Client::handle_read_status_line(const boost::system::error_code& err)
{

	if (!err)
    {
      // Check that response is OK.
      std::istream response_stream(&response_);
      std::string http_version;
      response_stream >> http_version;
      unsigned int status_code;
      response_stream >> status_code;
      std::string status_message;
      std::getline(response_stream, status_message);
      if (!response_stream || http_version.substr(0, 5) != "HTTP/")
      {
        std::cout << "Invalid response\n";
        return;
      }
      if (status_code != 200)
      {
        std::cout << "Response returned with status code ";
        std::cout << status_code << "\n";
        return;
      }

      // Read the response headers, which are terminated by a blank line.
      boost::asio::async_read_until(socket_, response_, "\r\n\r\n",
          boost::bind(&Client::handle_read_headers, this,
            boost::asio::placeholders::error));
    }
    else
    {
      std::cout << "Error: " << err << "\n";
    }
}



void Client::handle_read_headers(const boost::system::error_code& err)
{
    if (!err)
    {
      // Process the response headers.
      std::istream response_stream(&response_);
      std::string header;
      while (std::getline(response_stream, header) && header != "\r")
        std::cout << header << "\n";
      std::cout << "\n";

      // Write whatever content we already have to output.
     // if (response_.size() > 0)
     //   std::cout << &response_;

      // Start reading remaining data until EOF.
      boost::asio::async_read(socket_, response_,
          boost::asio::transfer_at_least(1),
          boost::bind(&Client::handle_read_content, this,
            boost::asio::placeholders::error));
    }
    else
    {
      std::cout << "Error: " << err << "\n";
    }

}


void Client::handle_read_content(const boost::system::error_code& err)
{

    if (!err)
    {
      // Write all of the data that has been read so far.
      std::ostringstream response;
      response << &response_;

      std::cout << response.str().c_str();

      json_error_t err_t;
      json_t* root;
      json_t* filePath;


      root = json_loads(response.str().c_str(), 0, &err_t);


      if (root == NULL){

    	  std::cout << "cannot load response" << err_t.text;
    	  return;
      }


      json_t * objects = json_object_get(root, "objects");

      if (objects == NULL){

    	  std::cout << "cannot find objects under root" << std::endl;

    	  filePath = json_object_get(root, "resource_uri");

      }
      else{

    	  if (json_is_array(objects)){

    		  filePath = json_object_get(json_array_get(objects, 0),"resource_uri");

    	  }

      }


      if (filePath != NULL){

    	  if (!json_is_array(filePath)){

			  reply_.status = http::server::reply::created;
			  reply_.headers.resize(1);
			  reply_.headers[0].name = "Location";
			  reply_.headers[0].value = json_string_value(filePath);

			  std::cout << "reply" << std::endl;
			  std::cout << reply_.headers[0].value;

			  //socket_.close();
			  return;
    	  }
      }


      // Continue reading remaining data until EOF.
      boost::asio::async_read(socket_, response_,
          boost::asio::transfer_at_least(1),
          boost::bind(&Client::handle_read_content, this,
            boost::asio::placeholders::error));
    }
    else if (err != boost::asio::error::eof)
    {
      std::cout << "Error: " << err << "\n";
    }
}
*/


