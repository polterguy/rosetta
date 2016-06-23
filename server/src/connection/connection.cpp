
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
#include "common/include/exceptional_executor.hpp"
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
{
  std::cout << "Connection created" << std::endl;
}


connection::~connection ()
{
  std::cout << "Connection destroyed" << std::endl;
}


void connection::handle()
{
  // Setting deadline timer to "keep-alive" value, to prevent a connection locking a thread on server.
  set_deadline_timer (_server->configuration().get<size_t> ("connection-keep-alive-timeout", 20));

  // Creating a copy of this, to make sure it stays around until request is finished.
  auto self = shared_from_this ();

  // Creating a new request and handling it, passing in shared_ptr to current connection.
  auto request = request::create (self);

  // Handling request, making sure we attach an exceptional_executor object that cleans up for us, in case of some exceptional occurring.
  // In addition, we hand in a copy of "self", and "request", to make sure they stay alive until request is finished.
  request->handle (exceptional_executor ([this, self, request] () {

    // Closing connection.
    close ();
  }));
}


void connection::set_deadline_timer (int seconds)
{
  return;
  if (seconds == -1) {

    // Canceling deadline timer, and returning immediately, without creating a new one.
    _timer.cancel();
  } else {

    // Updating the timer's expiration, which will implicitly invoke any existing handlers, with an "operation aborted" error code.
    _timer.expires_from_now (boost::posix_time::seconds (seconds));
    _timer.async_wait ([this] (const error_code & error) {

      // We don't close if the operation was aborted, since when timer is canceled, the handler will be invoked with
      // the "aborted" error_code, and every time we change the deadline timer, we implicitly cancel() any existing handlers.
      if (error != boost::asio::error::operation_aborted)
        close ();
    });
  }
}


void connection::close()
{
  // Killing deadline timer.
  _timer.cancel ();

  // Closing socket gracefully, if it is open.
  if (_socket->is_open ()) {

    // Socket still open, making sure we close it.
    error_code ignored_ec;
    _socket->shutdown (boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
    _socket->close();
  }

  // Making sure we delete connection from server's list of connections.
  _server->remove_connection (shared_from_this());
}


} // namespace server
} // namespace rosetta
