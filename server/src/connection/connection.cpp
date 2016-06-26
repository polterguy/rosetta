
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

#include "common/include/exceptional_executor.hpp"
#include "server/include/connection/request.hpp"
#include "server/include/connection/connection.hpp"

using namespace boost::asio;

namespace rosetta {
namespace server {


connection_ptr connection::create (class server * server, socket_ptr socket)
{
  return connection_ptr (new connection (server, socket));
}


connection::connection (class server * server, socket_ptr socket)
  : _server (server),
    _socket (socket),
    _timer (server->service())
{ }


void connection::handle()
{
  // Setting deadline timer to "keep-alive" value, to prevent a connection locking a thread on server, without ever sending envelope data.
  set_deadline_timer (_server->configuration().get<size_t> ("connection-keep-alive-timeout", 20));

  // Making sure we pass in a shared_ptr copy of this to function of exceptional_executor, to make sure connection is not destroyed,
  // before exceptional_executor for sure is released, or invoked.
  auto self = shared_from_this ();

  // Creating a new request on the current connection, and handle it, with an exceptional_executor giving guarantee of destroying the connection,
  // if an exception occurs.
  _request = request::create (this);
  _request->handle (exceptional_executor ([this, self] () {

    // Closing connection.
    ensure_close ();
  }), [this] (exceptional_executor x) {

    // Invoked when request is finished handled.
    // Releasing exceptional_executor, and invoking handle(), to wait for next request coming from client.
    x.release();
    handle();
  });
}


void connection::set_deadline_timer (int seconds)
{
  // Checking if caller only wants to destroy the current deadline timer, without creating a new.
  if (seconds == -1) {

    // Canceling deadline timer, and returning immediately, without creating a new one.
    _timer.cancel();
  } else {

    // Making sure we pass in a shared_ptr copy of this to wait handler of deadline timer, to make sure connection is not
    // destroyed before functions is destroyed, or invoked.
    auto self = shared_from_this ();

    // Updating the timer's expiration, which will implicitly invoke any existing handlers, with an "operation aborted" error code.
    _timer.expires_from_now (boost::posix_time::seconds (seconds));

    // Associating a handler with deadline timer, that ensures the closing of connection if it kicks in, unless timer is aborted.
    _timer.async_wait ([this, self] (auto error) {

      // We don't close if the operation was aborted, since when timer is canceled, the handler will be invoked with
      // the "aborted" error_code, and every time we change the deadline timer, we implicitly cancel() any existing handlers.
      if (error != error::operation_aborted)
        ensure_close ();
    });
  }
}


void connection::ensure_close()
{
  // Checking if connection is already on its way into the garbage.
  // This is necessary since in theory, multiple methods can attempt to destroy the connection simultaneously, due to our
  // logic with the exceptional_handler ensuring destruction of connection.
  if (_being_killed)
    return;

  // Signaling to future callers that this has already happened.
  _being_killed = true;

  // Killing deadline timer.
  _timer.cancel ();

  // Closing socket gracefully, if it is open.
  if (_socket->is_open ()) {

    // Socket is still open, making sure we close it, gracefully.
    error_code ec;
    _socket->shutdown (ip::tcp::socket::shutdown_both, ec);
    _socket->close();
  }

  // Making sure we delete connection from server's list of connections.
  // This will ensure that the last reference to the connection becomes invalidated, after all existing handlers,
  // keeping a reference to the shared_ptr have been invoked.
  _server->remove_connection (shared_from_this());
}


} // namespace server
} // namespace rosetta
