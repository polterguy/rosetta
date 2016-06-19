
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
#include "common/include/string_helper.hpp"
#include "common/include/date.hpp"
#include "server/include/connection/request.hpp"
#include "server/include/connection/connection.hpp"
#include "server/include/connection/match_condition.hpp"
#include "server/include/exceptions/request_exception.hpp"

namespace rosetta {
namespace server {
  
using std::move;
using std::vector;
using std::ifstream;
using std::shared_ptr;
using std::istreambuf_iterator;
using boost::split;
using boost::system::error_code;
using boost::posix_time::seconds;
using boost::algorithm::to_lower_copy;
using namespace rosetta::common;


connection_ptr connection::create (class server * server, socket_ptr socket)
{
  return connection_ptr (new connection (server, socket));
}


connection::connection (class server * server, socket_ptr socket)
  : _server (server),
    _socket (socket)
{ }


void connection::handle()
{
  // Creating a new request, passing in an exceptional_executor object, which unless the request somehow
  // makes sure release() is invoked on the exceptional_executor, then the connection is automatically closed.
  _request = request::create (this);

  // Handles the request.
  _request->handle (exceptional_executor ([this] () {

    // Closing connection.
    close ();
  }));
}


void connection::close()
{
  // Closing socket gracefully.
  error_code ignored_ec;
  _socket->shutdown (tcp::socket::shutdown_both, ignored_ec);
  _socket->close();

  // Making sure we delete connection from server's list of connections.
  _server->remove_connection (shared_from_this());
}


} // namespace server
} // namespace rosetta
