
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
#include "http_server/include/helpers/date.hpp"
#include "http_server/include/connection/request.hpp"
#include "http_server/include/connection/connection.hpp"
#include "http_server/include/exceptions/request_exception.hpp"
#include "http_server/include/connection/handlers/put_folder_handler.hpp"

namespace rosetta {
namespace http_server {

using std::string;
using namespace boost::filesystem;
using namespace rosetta::common;


put_folder_handler::put_folder_handler (class connection * connection, class request * request)
  : request_handler_base (connection, request)
{ }


void put_folder_handler::handle (exceptional_executor x, functor on_success)
{
  // Retrieving URI from request.
  auto path = request()->envelope().path();

  // Checking that folder does not exist.
  if (exists (path)) {

    // Oops, folder already exists.
    request()->write_error_response (x, 403);
  }

  // Creating folder.
  create_directories (path);

  // Returning success.
  write_success_envelope (x, on_success);
}


void put_folder_handler::write_success_envelope (exceptional_executor x, functor on_success)
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


} // namespace http_server
} // namespace rosetta
