
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


put_folder_handler::put_folder_handler (class request * request)
  : request_handler_base (request)
{ }


void put_folder_handler::handle (connection_ptr connection, std::function<void()> on_success)
{
  // Retrieving URI from request.
  auto path = request()->envelope().path();

  // Checking that folder does not exist.
  if (exists (path)) {

    // Oops, folder already exists.
    request()->write_error_response (connection, 500);
  } else {

    // Creating folder.
    create_directories (path);

    // Returning success.
    write_success_envelope (connection, on_success);
  }
}


void put_folder_handler::write_success_envelope (connection_ptr connection, std::function<void()> on_success)
{
  // Writing status code success back to client.
  write_status (connection, 200, [this, connection, on_success] () {

    // Writing standard headers back to client.
    write_standard_headers (connection, [this, connection, on_success] () {

      // Ensuring envelope is closed.
      ensure_envelope_finished (connection, on_success);
    });
  });
}


} // namespace http_server
} // namespace rosetta
