
/*
 * Rosetta web server, copyright(c) 2016, Thomas Hansen, phosphorusfive@gmail.com.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License, as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ROSETTA_SERVER_REQUEST_HPP
#define ROSETTA_SERVER_REQUEST_HPP

#include <map>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include "common/include/exceptional_executor.hpp"

namespace rosetta {
namespace server {

using std::map;
using std::string;
using std::function;
using std::shared_ptr;
using namespace rosetta::common;

class request;
typedef shared_ptr<request> request_ptr;

class request_handler;
typedef shared_ptr<request_handler> request_handler_ptr;

class connection;


/// Wraps a single HTTP request
class request
{
public:

  /// Constructs a new request of the specified type, path and version.
  static request_ptr create (connection * connection, boost::asio::streambuf & buffer);

  /// Handles a request by handling the entire body of the request, and then leave the execution up to the
  /// correct request_handler, according to the configuration of server.
  /// Notice, this method will "intelligently" try to figure out which resource is being requested, what type
  /// of method is being used, and which HTTP version is used, by using sane defaults, and allowing for severely
  /// malformed HTTP-Request lines and headers being sent. This is to allow "HTTP Cloaking Requests" to be sent
  /// by the client, that makes it harder, and even impossible in some cases, to even deduct which type of traffic
  /// the server and the client is engaging in, as a security feature, and counter measure of adversaries trying to
  /// intercept and decrypt the messages sent back and forth between the client and the server.
  void handle (exceptional_executor x);

  /// Returns URI of request.
  const string & uri() const { return _uri; }

  /// Returns the value of the HTTP header with the given name, or empty string, if there is no such header.
  const string & operator [] (const string & key) const;

  /// Creates and returns an error response, according to the specified status code.
  void write_error_response (int status_code, exceptional_executor x);

private:

  /// Constructs a new request of the specified type, path and version.
  request (connection * connection, boost::asio::streambuf & buffer);

  /// Decorates request according to initial HTTP-Request line sent, and returns true if request was successfully decorated.
  void decorate (const string & type,
                 const string & uri,
                 const string & version,
                 exceptional_executor x,
                 function<void(exceptional_executor x)> callback);

  /// Parses GET parameters.
  void parse_parameters (const string & params);

  /// Reads and parses the HTTP headers from the given connection.
  void read_headers (exceptional_executor x, function<void(exceptional_executor x)> callback);

  /// Reads and parses the HTTP headers from the given connection.
  void read_content (exceptional_executor x, function<void(exceptional_executor x)> callback);


  /// The connection this instance belongs to.
  connection * _connection;

  /// Type of request, GET/POST/DELETE/PUT etc.
  string _type;

  /// Path to resource requested.
  string _uri;

  /// HTTP version of request.
  string _version;

  /// Headers.
  map<string, string> _headers;

  /// HTTP parameters.
  map<string, string> _parameters;

  /// Request handler.
  request_handler_ptr _request_handler;

  /// Buffer for reading request.
  boost::asio::streambuf & _request_buffer;
};


} // namespace server
} // namespace rosetta

#endif // ROSETTA_SERVER_REQUEST_HPP
