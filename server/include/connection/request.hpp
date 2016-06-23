
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
#include <functional>
#include <boost/asio.hpp>
#include "common/include/exceptional_executor.hpp"
#include "server/include/connection/request_envelope.hpp"

namespace rosetta {
namespace server {

using std::string;

class request_handler;
typedef std::shared_ptr<request_handler> request_handler_ptr;

class connection;
typedef std::shared_ptr<connection> connection_ptr;

class request;
typedef std::shared_ptr<request> request_ptr;


/// Wraps a single HTTP request.
class request : boost::noncopyable
{
public:

  /// Creates a new request, and returns as a shared_ptr.
  static request_ptr create (connection_ptr connection);

  /// Handles a request, and invokes the given function when finished.
  void handle (exceptional_executor x);

  /// Returns the envelope of the request.
  const request_envelope & envelope() const { return _envelope; }

  /// Writes the given status code back to client.
  void write_error_response (exceptional_executor x, int status_code);

private:

  /// Creates an instance of this class on the given connection.
  request (connection_ptr connection);

  /// Reading content of request.
  void read_content (exceptional_executor x, std::function<void (exceptional_executor x)> functor);


  /// Connection this request belongs to.
  connection_ptr _connection;

  /// Envelope for request, HTTP-Request line and headers.
  request_envelope _envelope;

  /// Request handler, class responsible for taking correct action depending upon type/URI of request.
  request_handler_ptr _request_handler;
};


} // namespace server
} // namespace rosetta

#endif // ROSETTA_SERVER_REQUEST_HPP
