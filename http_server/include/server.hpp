
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
#include <map>
#include <memory>
#include <functional>
#include "common/include/configuration.hpp"
#include "http_server/include/auth/authorization.hpp"
#include "http_server/include/auth/authentication.hpp"
#include "http_server/include/connection/rosetta_socket.hpp"

using namespace boost::asio;
using namespace rosetta::common;

namespace rosetta {
namespace http_server {

class connection;
typedef std::shared_ptr<connection> connection_ptr;

typedef std::shared_ptr<rosetta_socket> socket_ptr;


/// This is the main server object, and there will only be one server running in your application.
class server final : public boost::noncopyable
{
public:

  /// Creates a server instance.
  server (const class configuration & configuration);

  /// Starts the server.
  void run ();

  /// Returns the configuration for our server.
  const class configuration & configuration () const { return _configuration; };

  /// Returns the io_service belonging to this instance.
  io_service & service () { return _service; }

  /// Removes the specified connection.
  void remove_connection (connection_ptr connection);

  /// Returns the authorization object for server
  const class authorization & authorization () const { return _authorization; }
  class authorization & authorization () { return _authorization; }

  /// Returns the authentication object for server
  const class authentication & authentication () const { return _authentication; }
  class authentication & authentication () { return _authentication; }

private:

  /// Starts a connection on the given socket.
  connection_ptr create_connection (socket_ptr socket);

  /// Sets up HTTP (non-SSL) server, to start accepting normal HTTP requests.
  void setup_http_server ();

  /// Sets up HTTPS (SSL) server, to start accepting Secure HTTPS requests.
  void setup_https_server ();

  /// Callback invoked when SIGINT/SIGTERM etc is signaled.
  void on_stop (int signal_number);

  /// Callback for accepting new HTTP connections.
  void on_accept();

  /// Callback for accepting new HTTPS connections.
  void on_accept_ssl();


  /// Only io service object in application.
  io_service _service;

  /// Configuration for server.
  const class configuration _configuration;

  /// The signal_set is used to register for process termination notifications.
  signal_set _signals;

  /// Acceptor which listens for incoming HTTP connections. (non-SSL requests)
  ip::tcp::acceptor _acceptor;

  /// Acceptor which listens for incoming HTTPS connections.
  ip::tcp::acceptor _acceptor_ssl;

  /// SSL context for SSL connections.
  ssl::context _context;

  /// All live connections to our server.
  std::map<ip::address, std::set<connection_ptr>> _connections;

  /// Authentication object for server.
  class authentication _authentication;

  /// Authorization object for server.
  class authorization _authorization;
};


} // namespace http_server
} // namespace rosetta

#endif // ROSETTA_SERVER_SERVER_HPP
