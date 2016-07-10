
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

#include <boost/filesystem.hpp>
#include "http_server/include/server.hpp"
#include "http_server/include/connection/request.hpp"
#include "http_server/include/connection/connection.hpp"
#include "http_server/include/connection/handlers/delete_handler.hpp"

namespace rosetta {
namespace http_server {

using std::string;
using namespace rosetta::common;


delete_handler::delete_handler (class request * request)
  : request_handler_base (request)
{ }


void delete_handler::handle (connection_ptr connection, std::function<void()> on_success)
{
  // Retrieving URI from request.
  auto path = request()->envelope().path();

  // Deleting file.
  boost::filesystem::remove (path);

  // Returning success to client.
  write_success_envelope (connection, on_success);
}


} // namespace http_server
} // namespace rosetta
