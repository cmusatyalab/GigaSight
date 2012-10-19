//
// server.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <boost/asio.hpp>
#include <string>
#include <boost/noncopyable.hpp>
#include "connection.hpp"
#include "connection_manager.hpp"
#include "request_handler.hpp"
#include "../ResourceManagement/SegmentInformation.h"
#include "../ResourceManagement/ProcessingRuleContainer.h"

namespace http {
namespace server {

/// The top-level class of the HTTP server.
class server
	: private boost::noncopyable
{
public:
  /// Construct the server to listen on the specified TCP address and port
  explicit server(const std::string& address, const std::string& port, SegmentInformation* s, ProcessingRuleContainer* c);

  /// Run the server's io_service loop.
  void run();

  /// Stop the server.
  void stop();

private:

  //port
  std::string listenPort;
  /// Handle completion of an asynchronous accept operation.
  void handle_accept(const boost::system::error_code& e);

  /// Handle a request to stop the server.
  void handle_stop();

  /// The io_service used to perform asynchronous operations.
  boost::asio::io_service io_service_;

  /// Acceptor used to listen for incoming connections.
  boost::asio::ip::tcp::acceptor acceptor_;

  /// The connection manager which owns all live connections.
  connection_manager connection_manager_;

  /// The next connection to be accepted.
  connection_ptr new_connection_;

  /// The handler for all incoming requests.
  request_handler request_handler_;

  bool bBinded;

  SegmentInformation* currentSegment;

  ProcessingRuleContainer* ruleManager;

  boost::asio::ip::tcp::endpoint endpoint;

  bool bindTCPPort();
};

} // namespace server
} // namespace http

#endif // HTTP_SERVER_HPP
