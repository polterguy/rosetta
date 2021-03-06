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
#include "http_server/include/server.hpp"
#include "http_server/include/connection/request.hpp"

namespace rosetta {
namespace http_server {

class connection;
typedef std::shared_ptr<connection> connection_ptr;


/// Wraps a connection to our server.
class connection final : public std::enable_shared_from_this<connection>, public boost::noncopyable
{
public:

  /// Factory method for creating a new connection.
  static connection_ptr create (class server * server, socket_ptr socket);

  /// Handles a connection to our server.
  void handle();

  /// Ensures connection is closed.
  void close();

  /// Sets the deadline timer for a specified amount of time, before connection is closed.
  void set_deadline_timer (int seconds = -1);

  /// Returns the server for the current instance.
  class server * server() { return _server; }
  const class server * server() const { return _server; }

  /// Returns the socket for the current instance.
  rosetta_socket & socket() { return *_socket; }

  /// Returns the address for the client.
  ip::address address() const { return _client_address; }

  /// Returns the stream buffer for the current instance.
  streambuf & buffer() { return _buffer; }

  /// Returns true if connection is SSL.
  bool is_secure() const { return _socket->is_secure (); };
  
private:

  /// Creates a connection on the given socket, for the given server instance.
  /// Private, to ensure only factory method can create instances.
  explicit connection (class server * server, socket_ptr socket);


  /// Server instance this connection belongs to.
  class server * _server;

  /// Socket for connection.
  socket_ptr _socket;

  /// Deadline timer for closing connection when a timeout period has elapsed.
  deadline_timer _timer;

  /// Request stream buffer.
  boost::asio::streambuf _buffer;

  /// Request for connection.
  request _request;
  
  /// Since we potentially need this one after socket is actually closed, we need to store it in here.
  ip::address _client_address;
};


} // namespace http_server
} // namespace rosetta

#endif // ROSETTA_SERVER_CONNECTION_HPP
