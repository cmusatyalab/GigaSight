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

namespace http {
namespace server {

server::server(const std::string& address, const std::string& port, SegmentInformation* s, ProcessingRuleContainer* c)
  : io_service_(),
    listenPort(port),
    acceptor_(io_service_),
    connection_manager_(),
    new_connection_(new connection(io_service_,
          connection_manager_, request_handler_)),
    request_handler_(),
    currentSegment(s),
    ruleManager(c),
    bBinded(false)
{

	// Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
	boost::asio::ip::tcp::resolver resolver(io_service_);
	boost::asio::ip::tcp::resolver::query query(address, port);
	endpoint = *resolver.resolve(query);

	acceptor_.open(endpoint.protocol());
	acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));

	request_handler_.configure(s, c);

	int currentPort;

	while (!bindTCPPort()){

		currentPort = endpoint.port() +1;
		endpoint.port(currentPort);
	}


	acceptor_.listen();

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

		std::cout << "HTTP server is listening on port " << listenPort << std::endl;
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

} // namespace server
} // namespace http
