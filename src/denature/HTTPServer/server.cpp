//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "server.hpp"
#include <boost/bind.hpp>
#include <jansson.h>

namespace http {
namespace server {

server::server(SharedQueue<SegmentInformation*>* s, ProcessingRuleContainer* c)
  : io_service_(),
    acceptor_(io_service_),
    connection_manager_(),
    new_connection_(new connection(io_service_,
          connection_manager_, request_handler_)),
    request_handler_(),
    segmentQueue(s),
    ruleManager(c),
    bBinded(false)
{

	loadConfiguration();

	// Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
	boost::asio::ip::tcp::resolver resolver(io_service_);
	std::cout << "host:" << host_ << ":" << listenPort << std::endl;
	boost::asio::ip::tcp::resolver::query query(host_, listenPort);
	endpoint = *resolver.resolve(query);

	acceptor_.open(endpoint.protocol());
	acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));

	request_handler_.configure(dmUri_, dmPort_, s, c);

	int currentPort;

	while (!bindTCPPort()){

		currentPort = endpoint.port() +1;
		endpoint.port(currentPort);

		if (currentPort - atoi(listenPort.c_str()) > 10){

			std::cout << "Is " << endpoint.address() <<  " the right host address" << std::endl;
			return;
		}
	}


	try{
		acceptor_.listen();
	}catch (boost::system::system_error& e) {

		std::cout << "acceptor_.listen() failed "<< e.what() <<  std::endl;

		return;
	}

	acceptor_.async_accept(new_connection_->socket(),
		boost::bind(&server::handle_accept, this,
        boost::asio::placeholders::error));
}

bool server::bindTCPPort(){
	try {
		acceptor_.bind(endpoint);
	}
	catch (boost::system::system_error& e) {

		std::cout << "TCP Port: "<< endpoint.port() <<" cannot be binded"<< std::endl;

		return false;
	}

	bBinded = true;

	std::cout << "TCP Port: "<< endpoint.port() <<" binded"<< std::endl;
	return true;
}


void server::run(){

	if (bBinded){

		std::cout << "HTTP server is listening on port " << endpoint.port() << std::endl;
		// The io_service::run() call will block until all asynchronous operations
		// have finished. While the server is running, there is always at least one
		// asynchronous operation outstanding: the asynchronous accept call waiting
		// for new incoming connections.
		io_service_.run();

		std::cout << "HTTP server stopped" << std::endl;
	}

}

void server::stop()
{

	// Post a call to the stop function so that server::stop() is safe to call

	// from any thread.
	io_service_.post(boost::bind(&server::handle_stop, this));

	std::cout << "HTTP server requested to stop" << std::endl;
}

void server::handle_accept(const boost::system::error_code& e)
{

	if (!e)
	{

		std::cout <<"handle_accept" << std::endl;
		connection_manager_.start(new_connection_);
		new_connection_.reset(new connection(io_service_, connection_manager_, request_handler_));

		acceptor_.async_accept(new_connection_->socket(),
				boost::bind(&server::handle_accept, this,
				boost::asio::placeholders::error));
   }
	else
		std::cout << "error in handle_accept" << e.message()  << std::endl;


}

void server::handle_stop()
{
	// The server is stopped by cancelling all outstanding asynchronous
	// operations. Once all operations have finished the io_service::run() call
	// will exit.
	acceptor_.close();
	connection_manager_.stop_all();

	bBinded = false;
}

void server::loadConfiguration(){

	json_error_t err_t;
	json_t* root = json_load_file("XMLFiles/config.json", 0, &err_t);
	json_t* address = NULL;
	json_t* port = NULL;
	json_t* dmAddr = NULL;
	json_t* dmPort = NULL;

	if (root != NULL){

		address = json_object_get(root, "PRIVATEVM_URI");

		//tcp port used by http server
		port = json_object_get(root, "PRIVATEVM_TCP_PORT");

		dmAddr = json_object_get(root, "DATAMANAGER_URI");

		dmPort = json_object_get(root, "DATAMANAGER_PORT");

	}

	if (address != NULL)
		host_ = json_string_value(address);
	else
		host_ = "127.0.0.1";

	if (port != NULL)
		listenPort = json_string_value(port);
	else
		listenPort = "12345";

	if (dmAddr != NULL)
		dmUri_ = json_string_value(dmAddr);
	else
		dmUri_ = "127.0.0.1";

	if ((dmPort != NULL) && (json_is_string(dmPort)))
		dmPort_ = json_string_value(dmPort);
	else
		dmPort_ = "5000";



	json_object_clear(dmAddr);
	json_object_clear(dmPort);
	json_object_clear(address);
	json_object_clear(port);
	json_object_clear(root);

	std::cout << "data manager: " << dmUri_ << ":" << dmPort_ << std::endl;

}


} // namespace server
} // namespace http
