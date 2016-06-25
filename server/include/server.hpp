
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

#ifndef ROSETTA_SERVER_SERVER_HPP
#define ROSETTA_SERVER_SERVER_HPP

#include <set>
#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include "common/include/configuration.hpp"

using namespace boost::asio;
using namespace rosetta::common;

namespace rosetta {
namespace server {

class connection;
typedef std::shared_ptr<connection> connection_ptr;

class server;
typedef std::shared_ptr<server> server_ptr;

typedef std::shared_ptr<ip::tcp::socket> socket_ptr;


/// This is the main server object, and there will only be one server running in your application.
class server : public boost::noncopyable
{
public:

  /// Creates a server instance.
  server (const class configuration & configuration);

  /// Creates a server object of type according to settings.
  static server_ptr create (const configuration & configuration);

  /// Starts the server.
  virtual void run ();

  /// Returns the configuration for our server.
  const class configuration & configuration () const { return _configuration; };

  /// Returns the io_service belonging to this instance.
  io_service & service () { return _service; }

  /// Removes the specified connection.
  virtual void remove_connection (connection_ptr connection);

protected:

  /// Starts a connection on the given socket.
  virtual connection_ptr create_connection (socket_ptr socket);

private:

  /// Callback invoked when SIGINT/SIGTERM etc is signaled.
  void on_stop (int signal_number);

  /// Callback for accepting new connections
  void on_accept();


  /// Only io service object in application.
  io_service _service;

  /// Configuration for server.
  const class configuration _configuration;

  /// The signal_set is used to register for process termination notifications.
  signal_set _signals;

  /// Acceptor which listens for incoming connections
  ip::tcp::acceptor _acceptor;

  /// All live connections to our server.
  std::set<connection_ptr> _connections;
};


} // namespace server
} // namespace rosetta

#endif // ROSETTA_SERVER_SERVER_HPP
