
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
#include "common/include/exceptional_executor.hpp"
#include "http_server/include/connection/request.hpp"
#include "http_server/include/connection/connection.hpp"

using namespace boost::asio;

namespace rosetta {
namespace http_server {


connection_ptr connection::create (class server * server, socket_ptr socket)
{
  return connection_ptr (new connection (server, socket));
}


connection::connection (class server * server, socket_ptr socket)
  : _server (server),
    _socket (socket),
    _timer (server->service()),
    _client_address (socket->remote_endpoint().address())
{ }


void connection::handle()
{
  // Setting deadline timer to "keep-alive" value.
  set_deadline_timer (_server->configuration().get<size_t> ("connection-keep-alive-timeout", 20));

  // Creating a new request on the current connection, and handling it.
  _request = request::create ();
  _request->handle (shared_from_this());
}


void connection::set_deadline_timer (int seconds)
{
  // Checking if caller only wants to destroy the current deadline timer, without creating a new.
  if (seconds == -1) {

    // Canceling deadline timer, and returning immediately, without creating a new one.
    _timer.cancel();
  } else {

    // Updating the timer's expiration, which will implicitly invoke any existing handlers, with an "operation aborted" error code.
    _timer.expires_from_now (boost::posix_time::seconds (seconds));

    // Associating a handler with deadline timer, that ensures the closing of connection if it kicks in, unless timer is aborted.
    _timer.async_wait ([this] (auto error) {

      // We don't close if the operation was aborted, since when timer is canceled, the handler will be invoked with
      // the "aborted" error_code, and every time we change the deadline timer, or cancel() the timer,
      // we implicitly invoke any existing handlers.
      if (error != error::operation_aborted)
        close ();
    });
  }
}


void connection::close()
{
  // Killing deadline timer, removing connection, and closing socket..
  _timer.cancel ();

  // Removing connection from server, which means that as async handlers are invoked, with an error, due to socket being closed,
  // all shared_ptrs will be destroyed, until there are no more of them left.
  _server->remove_connection (shared_from_this());

  // Closing socket gracefully, if it is open.
  if (_socket->is_open()) {

    // Socket still open, shutting down, and closing.
    boost::system::error_code ignored;
    _socket->shutdown(socket_base::shutdown_type::shutdown_both, ignored);
    _socket->close();
  }
}


} // namespace http_server
} // namespace rosetta
