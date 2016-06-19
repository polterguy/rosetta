
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

#ifndef ROSETTA_SERVER_CONNECTION_HPP
#define ROSETTA_SERVER_CONNECTION_HPP

#include <memory>
#include <boost/asio.hpp>
#include "common/include/exceptional_executor.hpp"
#include "server/include/server.hpp"

using std::string;
using boost::asio::ip::tcp;
using namespace boost::asio;

namespace rosetta {
namespace server {

class connection;
typedef shared_ptr<connection> connection_ptr;

class request;
typedef shared_ptr<request> request_ptr;


/// Wraps a single connection to our server, which means both the request and the response, and all associated objects.
class connection final : public std::enable_shared_from_this<connection>, public boost::noncopyable
{
public:

  /// Making request a friend class, such that it can access the socket,
  /// server, streambuf, and other private data members.
  friend class request;

  /// Tracking destruction of connections.
  ~connection ();

  /// Creates a new connection.
  static connection_ptr create (class server * server, socket_ptr socket);

  /// Returns the socket for the current connection.
  socket_ptr socket() { return _socket; }

  /// Handles a connection to our server.
  void handle ();

  /// Stops a connection.
  void stop ();

  /// Creates and returns an error response, according to the specified status code.
  void write_error_response (int status_code, exceptional_executor x);

private:

  /// Returns an exception_executor object that ensures closing of connection unless release() is invoked on the return value.
  exceptional_executor ensure_close ();

  /// Creates a connection belonging to the specified connection_manager, on the given socket.
  explicit connection (class server * server, socket_ptr socket);


  /// Server instance this connection is running on.
  server * _server;

  /// Socket for connection.
  socket_ptr _socket;

  /// Buffer for reading request.
  streambuf _request_buffer;

  /// Contains a reference to the request.
  request_ptr _request;
};


} // namespace server
} // namespace rosetta

#endif // ROSETTA_SERVER_CONNECTION_HPP
