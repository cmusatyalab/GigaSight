//
// connection.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "connection.hpp"
#include <vector>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include "connection_manager.hpp"
#include "request_handler.hpp"

namespace http {
namespace server {

connection::connection(boost::asio::io_service& io_service,
    connection_manager& manager, request_handler& handler)
  : socket_(io_service),
    connection_manager_(manager),
    request_handler_(handler)
{
	partStr = "";
	receivedBytes = 0;
}

boost::asio::ip::tcp::socket& connection::socket()
{
  return socket_;
}

void connection::start()
{


	socket_.async_receive(boost::asio::buffer(buffer_),
      boost::bind(&connection::handle_read, shared_from_this(),
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));

  /*socket_.async_read_some(boost::asio::buffer(buffer_),
      boost::bind(&connection::handle_read, shared_from_this(),
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));*/
}

void connection::stop()
{
  socket_.close();
}

void connection::handle_read(const boost::system::error_code& e,
    std::size_t bytes_transferred)
{

	bool bComplete = false;

	if (!e)
	{

		boost::tribool result;

		boost::tie(result, boost::tuples::ignore) = request_parser_.parse(
				request_, buffer_.data(), buffer_.data() + bytes_transferred);

		//std::cout << "bytes_transferred " << bytes_transferred << std::endl;

		receivedBytes += bytes_transferred;

		//added: parse the body of the message if Content-Length > 0
		//todo: set the result variable
		for(unsigned int i = 0; i < request_.headers.size(); i++){

			if(!(request_.headers[i].name.compare("Content-Length"))){

				int contentLength = boost::lexical_cast<int>(request_.headers[i].value);

				//std::cout << "Content-Length " << contentLength << std::endl;

				if (contentLength > receivedBytes){
					//std::cout << "part of data has not arrived " << std::endl;

					partStr.append(buffer_.data());

					buffer_.assign(0);

					socket_.async_read_some(boost::asio::buffer(buffer_),
    			           boost::bind(&connection::handle_read, shared_from_this(),
    			             boost::asio::placeholders::error,
    			             boost::asio::placeholders::bytes_transferred));
					return;
				}


				if (!partStr.empty()){

					partStr.append(buffer_.data());

					//std::cout << partStr.data() << std::endl;

					if (!request_parser_.parseBody(request_, partStr.c_str(), contentLength)){

						partStr.assign(0);
						buffer_.assign(0);

						return;
					} else
						bComplete = true;

				} else{

					if (!request_parser_.parseBody(request_, buffer_.data(), contentLength)){
    					partStr.assign(0);
    					buffer_.assign(0);
    					return;
					}else
						bComplete = true;

				}

    		break;
    	}
    }


    if (bComplete)
    {

    	request_handler_.handle_request(request_, reply_);

    	request_.body.clear();//shall we clear it here
    	partStr.clear();
    	buffer_.assign(0);

    	bComplete = false;

    	receivedBytes = 0;

    	boost::asio::async_write(socket_, reply_.to_buffers(),
          boost::bind(&connection::handle_write, shared_from_this(),
            boost::asio::placeholders::error));


    }
    else if (!result)
    {


    	partStr.clear();

    	buffer_.assign(0);

    	receivedBytes = 0;

    	reply_ = reply::stock_reply(reply::bad_request);

    	boost::asio::async_write(socket_, reply_.to_buffers(),
          boost::bind(&connection::handle_write, shared_from_this(),
            boost::asio::placeholders::error));
    }
    else
    {

    	socket_.async_read_some(boost::asio::buffer(buffer_),
          boost::bind(&connection::handle_read, shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));


    }
  }
  else if (e != boost::asio::error::operation_aborted)
  {
    connection_manager_.stop(shared_from_this());
  }
}

void connection::handle_write(const boost::system::error_code& e)
{
  if (!e)
  {
    // Initiate graceful connection closure.
    boost::system::error_code ignored_ec;
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
  }

  if (e != boost::asio::error::operation_aborted)
  {
    connection_manager_.stop(shared_from_this());
  }
}

} // namespace server
} // namespace http
