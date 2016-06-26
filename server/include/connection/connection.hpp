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
#include "server/include/server.hpp"

using std::string;
using namespace boost::asio;
using namespace rosetta::common;

namespace rosetta {
namespace server {

class connection;
typedef std::shared_ptr<connection> connection_ptr;

class request;
typedef std::shared_ptr<request> request_ptr;


/// Wraps a single connection to our server, which might include multiple requests, and their associated response.
class connection final : public std::enable_shared_from_this<connection>, public boost::noncopyable
{
public:

  /// Creates a new connection.
  static connection_ptr create (class server * server, socket_ptr socket);

  /// Handles a connection to our server.
  void handle();

  /// Close the connection.
  void ensure_close();

  /// Sets the deadline timer for a specified amount of time, before connection is closed.
  void set_deadline_timer (int seconds = -1);

  /// Returns the server for the current instance.
  const server * server() const { return _server; }

  /// Returns the socket for the current instance.
  rosetta_socket & socket() { return *_socket; }

  /// Returns the stream buffer for the current instance.
  streambuf & buffer() { return _buffer; }

  /// Returns true if connection is SSL.
  bool is_secure() const { return _socket->is_secure (); };

private:

  /// Creates a connection on the given socket, for the given server instance.
  explicit connection (class server * server, socket_ptr socket);

  /// Server instance this connection belongs to.
  class server * _server;

  /// Socket for connection.
  socket_ptr _socket;

  /// Deadline timer, used to close connection, if a timeout occurs.
  deadline_timer _timer;

  /// ASIO request stream buffer, kept by connection, to support HTTP pipelining, across multiple requests.
  streambuf _buffer;

  /// Request for connection.
  request_ptr _request;

  /// True if connection is on its way to be killed.
  bool _killed = false;
};


} // namespace server
} // namespace rosetta

#endif // ROSETTA_SERVER_CONNECTION_HPP
