
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

#ifndef ROSETTA_SERVER_REQUEST_HANDLER_HPP
#define ROSETTA_SERVER_REQUEST_HANDLER_HPP

#include <tuple>
#include <memory>
#include <vector>
#include <utility>
#include <boost/asio.hpp>
#include "common/include/exceptional_executor.hpp"

namespace rosetta {
namespace server {

using std::tuple;
using std::string;
using std::vector;
using std::function;
using std::shared_ptr;
using namespace rosetta::common;

class server;
class request;
class connection;
class request_handler;
typedef shared_ptr<request_handler> request_handler_ptr;


/// Handles an HTTP request.
class request_handler : public boost::noncopyable
{
public:

  /// Creates the specified type of handler, according to file extension given, and configuration of server.
  static request_handler_ptr create (server * server, connection * connection, request * request, const string & extension);

  /// Handles the given request.
  virtual void handle (exceptional_executor x, function<void (exceptional_executor x)> callback) = 0;

protected:

  /// Protected constructor, to make sure only factory method can create instances.
  request_handler (server * server, connection * connection, request * request);

  /// Writing given HTTP headetr with given value back to client.
  void write_status (unsigned int status_code, exceptional_executor x, function<void (exceptional_executor x)> callback);

  /// Writing given HTTP header with given value back to client.
  void write_header (const string & key, const string & value, exceptional_executor x, function<void (exceptional_executor x)> callback);

  /// Writing given HTTP headers with given value back to client.
  void write_headers (vector<tuple<string, string> > headers, exceptional_executor x, function<void (exceptional_executor x)> callback);


  /// The server object.
  server * _server;

  /// The connection to the current instance.
  connection * _connection;

  /// The request that owns this instance.
  request * _request;
};


} // namespace server
} // namespace rosetta

#endif // ROSETTA_SERVER_REQUEST_HANDLER_HPP
