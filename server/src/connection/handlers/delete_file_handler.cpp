
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
#include "server/include/server.hpp"
#include "server/include/connection/request.hpp"
#include "server/include/connection/connection.hpp"
#include "server/include/connection/handlers/delete_file_handler.hpp"

namespace rosetta {
namespace server {

using std::string;
using namespace rosetta::common;


delete_file_handler::delete_file_handler (class connection * connection, class request * request)
  : request_handler (connection, request)
{ }


void delete_file_handler::handle (exceptional_executor x, functor on_success)
{
  // Retrieving URI from request, removing initial "/", before prepending it with the www-root folder.
  string uri = request()->envelope().uri().substr (1);

  // Retrieving root path, and building full path for document.
  const string WWW_ROOT_PATH = connection()->server()->configuration().get<string> ("www-root", "www-root/");
  const string filename = WWW_ROOT_PATH + uri;

  // Deleting file.
  boost::filesystem::remove (filename);

  // Returning success to client.
  write_success_envelope (x, on_success);
}


void delete_file_handler::write_success_envelope (exceptional_executor x, functor on_success)
{
  // Writing status code success back to client.
  write_status (200, x, [this, on_success] (auto x) {

    // Writing standard headers back to client.
    write_standard_headers (x, [this, on_success] (auto x) {

      // Ensuring envelope is closed.
      ensure_envelope_finished (x, on_success);
    });
  });
}


} // namespace server
} // namespace rosetta
