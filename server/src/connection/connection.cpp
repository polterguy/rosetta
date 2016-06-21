
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

#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "common/include/date.hpp"
#include "server/include/connection/request.hpp"
#include "server/include/connection/connection.hpp"
#include "server/include/exceptions/request_exception.hpp"

namespace rosetta {
namespace server {

using boost::system::error_code;
using namespace rosetta::common;


connection_ptr connection::create (class server * server, socket_ptr socket)
{
  return connection_ptr (new connection (server, socket));
}


connection::connection (class server * server, socket_ptr socket)
  : _server (server),
    _socket (socket),
    _timer (server->io_service())
{ }


connection::~connection ()
{
  // Killing deadline timer.
  kill_deadline_timer ();

  // Closing socket gracefully, if it is open.
  if (_socket->is_open ()) {

    // Socket still open, making sure we close it.
    error_code ignored_ec;
    _socket->shutdown (boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
    _socket->close();
  }
}


void connection::handle()
{
  // Settings deadline timer to "keep-alive" value, which means that if the client cannot send
  // the initial HTTP-Request line in less than "keep-alive" amount of seconds,
  // then the connection times out, also on the first initial request that created the connection.
  // This prevents a client in establishing a connection, and never send any bytes, and such "lock" a connection,
  // eating up resources on the server.
  set_deadline_timer (_server->configuration().get<size_t> ("connection-keep-alive-timeout", 5));

  // Creating a new request, passing in an exceptional_executor object, which unless the request somehow
  // makes sure release() is invoked on the exceptional_executor, then the connection is automatically closed.
  _request = request::create (this, _request_buffer);
  _request->handle (exceptional_executor ([this] () {

    // Closing connection.
    close ();
  }));
}


void connection::keep_alive ()
{
  // Creating a new request, and handling it.
  // Notice, this will keep our socket open, and also keep the stream buffer, which allows for "keep-alive" connections,
  // while also supporting "pipelining" of requests.
  handle ();
}


void connection::close()
{
  // Making sure we delete connection from server's list of connections.
  // This will delete the last reference to the connection's shared_ptr,
  // and hence make sure the destructor is invoked, which will clean up everything.
  _server->remove_connection (shared_from_this());
}


void connection::set_deadline_timer (size_t seconds)
{
  // Updating the timer's expiration, which will implicitly invoke any existing handlers, with an "operation aborted" error code.
  _timer.expires_from_now (boost::posix_time::seconds (seconds));
  _timer.async_wait ([this] (const error_code & error) {

    // We don't close if the operation was aborted, since when timer is canceled, the handler will be invoked with
    // the "aborted" error_code, and every time we change the deadline timer, we implicitly cancel() any existing handlers.
    if (error != boost::asio::error::operation_aborted)
      close ();
  });
}


void connection::kill_deadline_timer ()
{
  _timer.cancel ();
}


} // namespace server
} // namespace rosetta
