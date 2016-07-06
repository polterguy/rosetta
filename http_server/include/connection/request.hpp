
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

#include <memory>
#include "common/include/exceptional_executor.hpp"
#include "http_server/include/connection/request_envelope.hpp"
#include "http_server/include/connection/create_request_handler.hpp"
#include "http_server/include/connection/handlers/request_handler_base.hpp"

namespace rosetta {
namespace http_server {

class connection;
typedef std::shared_ptr<connection> connection_ptr;

class request;
typedef std::shared_ptr<request> request_ptr;


/// Wraps a single HTTP request.
class request : public std::enable_shared_from_this<request>, public boost::noncopyable
{
public:

  /// Creates a new request, and returns as a shared_ptr.
  static request_ptr create (connection_ptr connection);

  /// Handles a request, and invokes the given function when finished.
  void handle ();

  /// Returns the envelope of the request.
  const request_envelope & envelope() const { return _envelope; }

  /// Writes the given error response back to client.
  void write_error_response (int status_code);

private:

  /// Creates an instance of this class on the given connection. Private to ensure factory method is used.
  request (connection_ptr connection);


  /// Connection this request belongs to.
  connection_ptr _connection;

  /// Envelope for request, HTTP-Request line, HTTP headers and GET parameters.
  request_envelope _envelope;

  /// Request handler, class responsible for taking correct action depending upon type/URI of request.
  request_handler_ptr _request_handler;
};


} // namespace http_server
} // namespace rosetta

#endif // ROSETTA_SERVER_REQUEST_HPP
