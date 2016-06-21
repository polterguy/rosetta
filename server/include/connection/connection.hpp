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
#include <boost/date_time/posix_time/posix_time.hpp>
#include "common/include/exceptional_executor.hpp"
#include "server/include/server.hpp"

using std::string;

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

  /// Destroys the connection.
  ~connection ();

  /// Handles a connection to our server.
  void handle ();

  /// Keeps a connection alive for consecutive requests, for a configured amount of time.
  void keep_alive ();

  /// Close the connection.
  void close ();

  /// Returns the socket for the current connection.
  socket_ptr socket() { return _socket; }

private:

  /// Making request a friend class, such that it can access the socket, server, stream buffer, and other private data members.
  friend class request;

  /// Creates a connection on the given socket, for the given server instance.
  explicit connection (class server * server, socket_ptr socket);

  /// Sets the deadline timer for a specified amount of time, before connection is closed.
  void set_deadline_timer (size_t seconds);

  /// Kills the deadline timer altogether.
  void kill_deadline_timer ();


  /// Server instance this connection belongs to.
  server * _server;

  /// Socket for connection.
  socket_ptr _socket;

  /// The request, will be created, and handled, when handle() on the connection is invoked.
  request_ptr _request;

  /// Deadline timer, used to close connection, if a timeout occurs.
  boost::asio::deadline_timer _timer;

  /// ASIO stream buffer, kept by connection, to support HTTP pipelining, across multiple requests.
  boost::asio::streambuf _request_buffer;
};


} // namespace server
} // namespace rosetta

#endif // ROSETTA_SERVER_CONNECTION_HPP
