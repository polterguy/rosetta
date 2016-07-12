
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

#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "http_server/include/server.hpp"
#include "http_server/include/connection/connection.hpp"
#include "http_server/include/exceptions/request_exception.hpp"

using std::string;
using boost::system::error_code;
using namespace boost::filesystem;

namespace rosetta {
namespace http_server {
  
// Configuration keys used to retrieve configuration options for our server object
static char const * const ADDRESS_CONFIG_KEY = "address";
static char const * const PORT_CONFIG_KEY = "port";
static char const * const SSL_PORT_CONFIG_KEY = "ssl-port";
static char const * const CERT_FILE = "ssl-certificate";
static char const * const PRIVATE_KEY_FILE = "ssl-private-key";
static char const * const SSL_HANDSHAKE_TIMEOUT = "connection-ssl-handshake-timeout";


server::server (const class configuration & configuration)
  : _configuration (configuration),
    _signals (_service),
    _acceptor (_service),
    _acceptor_ssl (_service),
    _context (ssl::context::sslv23),
    _authorization (configuration.get<path> ("www-root", "www-root"))
{
  // Register quit signals.
  _signals.add (SIGINT);
  _signals.add (SIGTERM);
#if defined(SIGQUIT)
  _signals.add (SIGQUIT);
#endif // defined(SIGQUIT)

  // Registering handle_stop as callback for any of the above signals.
  _signals.async_wait ([this] (const error_code & er, int signal_number){

    // Stopping server.
    on_stop (signal_number);
  });

  // Try to setup server to accept non-SSL, normal HTTP requests.
  setup_http_server ();

  // Try to setup server to accept SSL, HTTPS requests.
  setup_https_server ();
}


void server::run ()
{
  // To deal with exceptions, on a per thread basis, we need to deal with them here, since io:service.run()
  // is a blocking operation, and potential exceptions will be thrown from async handlers.
  // This means our exceptions will propagate all the way out here.
  // To deal with them, we catch everything here, and simply restart our io_service.
  while (true) {
    
    try {
      
      // This will block the current thread until all jobs are finished.
      // Since there's always another job in our queue, it will never return in fact, until a stop signal is given,
      // such as SIGINT or SIGTERM etc, or an exception occurs.
      _service.run ();
      
      // This is an attempt to gracefully shutdown the server, simply break while loop.
      break;
    } catch (std::exception & error) {
      
      // An exception occurred, simply re-iterating while loop, to re-start io_service.
      // We could do logging here, especially for debugging purposes.
      // If you wish to log, then please un-comment the following line.
      std::cerr << error.what () << std::endl;
    }
  }
}


connection_ptr server::create_connection (socket_ptr socket)
{
  // Figuring out IP address for current connection.
  ip::address client_address = socket->remote_endpoint().address();

  // Retrieving a reference to the existing set of connections for client's IP address.
  // If there are no existing connection, then a new set will be created.
  auto & client_connections = _connections [client_address];

  // Checking if server is configured to only allow a maximum number of connections per client.
  const int max_connections_per_client = configuration().get<int> ("max-connections-per-client", 8);
  if (max_connections_per_client != -1) {

    // Checking if the number of connections for IP address exceeds our max value, and if so, we refuse the connection.
    if (client_connections.size() >= static_cast<size_t> (max_connections_per_client)) {

      // We refuse this connection.
      throw request_exception ("Client has too many connections.");
    }
  }

  // Creating a new connection as a shared pointer, and putting it into our list of connections.
  connection_ptr connection = connection::create (this, socket);
  socket->set_connection (connection);
  client_connections.insert (connection);
  return connection;
}


void server::remove_connection (connection_ptr connection)
{
  // Erasing connection from our list of connections.
  ip::address client_address = connection->address();
  auto & client_connections = _connections [client_address];
  client_connections.erase (connection);

  // Checking if this is the last connection from the client, and if so, entirely erasing client's connections from set
  if (client_connections.size() == 0)
    _connections.erase (client_address);
}


void server::setup_http_server ()
{
  // Figuring out port to use for normal HTTP requests, if any.
  string port = _configuration.get<string> (PORT_CONFIG_KEY, "-1");
  if (port == "-1")
    return; // No HTTP traffic is accepted!

  // Figuring out address and port to start endpoint for.
  string address = _configuration.get<string> (ADDRESS_CONFIG_KEY, "localhost");

  // Resolving address and port, for then to open endpoint.
  ip::tcp::resolver resolver (_service);
  ip::tcp::endpoint endpoint = *resolver.resolve ({address, port});

  // Letting endpoint decide whether or not we should use IP version 4 or 6.
  _acceptor.open (endpoint.protocol());

  // Allowing the acceptor to reuse address, before binding to endpoint.
  _acceptor.set_option (ip::tcp::acceptor::reuse_address (true));
  _acceptor.bind (endpoint);

  // Start listening on acceptor.
  _acceptor.listen();

  // Start accepting connections.
  on_accept();
}


void server::setup_https_server ()
{
  // Figuring out port to use for normal HTTP requests, if any.
  string port = _configuration.get<string> (SSL_PORT_CONFIG_KEY, "-1");
  if (port == "-1")
    return; // No HTTPS traffic is accepted!

  // Associating certificate and private key with SSL context.
  const string certificate = _configuration.get<string> (CERT_FILE, "server.crt");
  const string private_key = _configuration.get<string> (PRIVATE_KEY_FILE, "server.key");
  _context.use_certificate_chain_file (certificate);
  _context.use_private_key_file (private_key, ssl::context::pem);

  // Figuring out address and port to start endpoint for
  string address = _configuration.get<string> (ADDRESS_CONFIG_KEY, "localhost");

  // Resolving address and port, for then to open endpoint
  ip::tcp::resolver resolver (_service);
  ip::tcp::endpoint endpoint = *resolver.resolve ({address, port});

  // Letting endpoint decide whether or not we should use IP version 4 or 6
  _acceptor_ssl.open (endpoint.protocol());

  // Allowing the acceptor to reuse address, before binding to endpoint
  _acceptor_ssl.set_option (ip::tcp::acceptor::reuse_address (true));
  _acceptor_ssl.bind (endpoint);

  // Start listening on acceptor.
  _acceptor_ssl.listen();

  // Start accepting connections.
  on_accept_ssl();
}


void server::on_accept ()
{
  // Waiting for next request.
  auto socket_ptr = std::make_shared<rosetta_socket_plain> (_service);
  _acceptor.async_accept (socket_ptr->socket(), [this, socket_ptr] (const error_code & error) {

    // Invoking "self" again to accept next request.
    on_accept();

    // Checking that our acceptor is still open, and not killed
    if (!_acceptor.is_open ())
      return;

    if (!error) {

      // Creating connection and handling it.
      create_connection (socket_ptr)->handle();
    }
  });
}


void server::on_accept_ssl ()
{
  // Waiting for next request.
  auto socket = std::make_shared<rosetta_socket_ssl> (_service, _context);
  _acceptor_ssl.async_accept (socket->ssl_stream().lowest_layer (), [this, socket] (const error_code & error) {

    // Invoking "self" again to accept next request.
    on_accept_ssl ();

    // Checking that our acceptor is still open, and not killed
    if (!_acceptor_ssl.is_open ())
      return;

    if (!error) {

      // Settings options for SSL socket.
      ip::tcp::no_delay opt (true);
      socket->ssl_stream().lowest_layer().set_option (opt);

      // Making sure we timeout handshake, to not lock up resources, with a handshake that never comes.
      int seconds = _configuration.get<int> (SSL_HANDSHAKE_TIMEOUT, 5);
      std::shared_ptr<deadline_timer> handshake_timer = std::make_shared<deadline_timer> (_service);
      handshake_timer->expires_from_now (boost::posix_time::seconds (seconds));
      handshake_timer->async_wait ([handshake_timer, socket] (const error_code & error) {

        // Checking that operation was not aborted.
        if (error != error::operation_aborted) {

          // Closing socket and cleaning up. Client spent too much time on handshake!
          error_code ec;
          socket->shutdown (ip::tcp::socket::shutdown_both, ec);
          socket->close();
        }
      });

      // Doing SSL handshake.
      socket->ssl_stream().async_handshake (ssl::stream_base::server, [this, socket, handshake_timer] (const error_code & error) {

        // Verifying nothing went sour.
        if (!error) {

          // Canceling handshake timeout.
          handshake_timer->cancel ();

          // Creating connection and handling it.
          create_connection (socket)->handle();
        }
      });
    }
  });
}


void server::on_stop (int signal_number)
{
  // Making sure we do not accept anymore incoming requests.
  _acceptor.close ();

  // Closing all open connections.
  for (auto idxClient : _connections) {

    // Stopping all connections for client first, then removing client entirely from list of IP addresses.
    for (auto idxConnection : idxClient.second) {
      idxConnection->close ();
    }
    _connections.erase (idxClient.first);
  }
}


} // namespace http_server
} // namespace rosetta
