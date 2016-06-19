
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

  /// Creates a new connection.
  static connection_ptr create (class server * server, socket_ptr socket);

  /// Destroys the connection.
  ~connection ();

  /// Handles a connection to our server.
  void handle ();

  /// Keeps a connection alive for consecutive requests for an n amount of configurable seconds.
  void keep_alive ();

  /// Stops a connection.
  void close ();

  /// Returns the socket for the current connection.
  socket_ptr socket() { return _socket; }

private:

  /// Making request a friend class, such that it can access the socket,
  /// server, streambuf, and other private data members.
  friend class request;

  /// Creates a connection belonging to the specified connection_manager, on the given socket.
  explicit connection (class server * server, socket_ptr socket);

  /// Sets the deadline timer for a specified amount of time before connection is closed unless timer is retouched.
  void set_deadline_timer (boost::posix_time::seconds seconds);

  /// Kills deadline timer altogether.
  void kill_deadline_timer ();


  /// Server instance this connection is running on.
  server * _server;

  /// Socket for connection.
  socket_ptr _socket;

  /// Contains a reference to the request.
  request_ptr _request;

  /// Deadline timer, used to close connection if a timeout occurs.
  deadline_timer _timer;
};


} // namespace server
} // namespace rosetta

#endif // ROSETTA_SERVER_CONNECTION_HPP
