
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
#include "server/include/connection/request_envelope.hpp"
#include "server/include/connection/handlers/request_handler.hpp"

namespace rosetta {
namespace server {

using std::string;

class connection;

class request;
typedef std::shared_ptr<request> request_ptr;


/// Wraps a single HTTP request.
class request : public std::enable_shared_from_this<request>, public boost::noncopyable
{
public:

  /// Creates a new request, and returns as a shared_ptr.
  static request_ptr create (connection * connection);

  /// Handles a request, and invokes the given function when finished.
  void handle (exceptional_executor x);

  /// Returns the envelope of the request.
  const request_envelope & envelope() const { return _envelope; }

  /// Writes the given status code back to client.
  void write_error_response (exceptional_executor x, int status_code);

private:

  /// Creates an instance of this class on the given connection.
  request (connection * connection);

  /// Reading content of request.
  void ensure_read_content (exceptional_executor x, functor callback);


  /// Connection this request belongs to.
  connection * _connection;

  /// Envelope for request, HTTP-Request line and headers.
  request_envelope _envelope;

  /// Request handler, class responsible for taking correct action depending upon type/URI of request.
  request_handler_ptr _request_handler;

  /// True if content has already been read, otherwise false.
  bool _content_has_been_read = false;
};


} // namespace server
} // namespace rosetta

#endif // ROSETTA_SERVER_REQUEST_HPP
