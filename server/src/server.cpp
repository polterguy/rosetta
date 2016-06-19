
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

#include <utility>
#include "common/include/rosetta_exception.hpp"
#include "common/include/configuration_exception.hpp"
#include "server/include/server.hpp"
#include "server/include/multi_thread_server.hpp"
#include "server/include/single_thread_server.hpp"
#include "server/include/connection/connection.hpp"

using std::move;
using std::string;
using std::make_shared;
using namespace boost::system;
using namespace boost::asio::ip;

namespace rosetta {
namespace server {
  
// Configuration keys used to retrieve configuration options for our server object
static char const * const ADDRESS_CONFIG_KEY = "address";
static char const * const PORT_CONFIG_KEY = "port";
static char const * const THREAD_MODEL_CONFIG_KEY = "thread-model";


server_ptr server::create (const class configuration & configuration)
{
  string thread_model = configuration.get<string> (THREAD_MODEL_CONFIG_KEY, "thread-model");

  if (thread_model == "single-thread") {

    // Single threaded server.
    return server_ptr (make_shared<single_thread_server> (configuration));
  } else if (thread_model == "thread-pool") {

    // Thread pool server.
    return server_ptr (make_shared<thread_pool_server> (configuration));
  } else {

    // Unknown server thread model.
    throw configuration_exception ("Unknown thread-model setting found in configuration file; '" + thread_model + "'");
  }
}


server::server (const class configuration & configuration)
  : _configuration (configuration),
    _signals (_service),
    _acceptor (_service)
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
  
  // Figuring out address and port to start endpoint for
  string address = _configuration.get<string> (ADDRESS_CONFIG_KEY, "localhost");
  string port    = _configuration.get<string> (PORT_CONFIG_KEY, "8080");
  
  // Resolving address and port, for then to open endpoint
  tcp::resolver resolver (_service);
  tcp::endpoint endpoint = *resolver.resolve ({address, port});
  
  // Letting endpoint decide whether or not we should use IP version 4 or 6
  _acceptor.open (endpoint.protocol());
  
  // Allowing the acceptor to reuse address, before binding to endpoint
  _acceptor.set_option(tcp::acceptor::reuse_address (true));
  _acceptor.bind (endpoint);
  
  // Start listening on acceptor.
  _acceptor.listen();

  // Start accepting connections.
  on_accept();
}


connection_ptr server::create_connection (socket_ptr socket)
{
  // Creating a new connection as a shared pointer, and putting it into our list of connections.
  connection_ptr connection = connection::create (this, socket);
  _connections.insert (connection);
  return connection;
}


void server::remove_connection (connection_ptr connection)
{
  // Since our connection is a shared pointer, removing it from our list of connections,
  // should destroy the last reference to it.
  _connections.erase (connection);
}


void server::on_accept ()
{
  // Waiting for next request.
  auto socket = make_shared<tcp::socket> (io_service ());
  _acceptor.async_accept(*socket, [this, socket] (const error_code & error) {

    // Checking that our acceptor is still open, and not killed
    if (!_acceptor.is_open ())
      return;

    if (!error) {

      // Handle request
      auto connection = create_connection (socket);
      connection->handle ();
    }

    // Invoking "self" again to accept next request.
    on_accept();
  });
}


void server::on_stop (int signal_number)
{
  // Making sure we do not accept anymore incoming requests.
  _acceptor.close ();

  // Closing all open connections.
  for (auto idx = _connections.begin(); idx != _connections.end(); idx++) {

    // Stopping connection first, then removing from list of open connections.
    (*idx)->stop ();
    remove_connection (*idx);
  }
}


} // namespace server
} // namespace rosetta
