
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
#include "http_server/include/connection/handlers/meta/head_handler.hpp"

namespace rosetta {
namespace http_server {

using std::string;
using namespace rosetta::common;


/// Verifies URI is sane, and not malformed, attempting to retrieve document outside of main folder, hidden files, etc.
bool sanity_check_uri (path uri);


head_handler::head_handler (class connection * connection, class request * request)
  : request_handler (connection, request)
{ }


void head_handler::handle (exceptional_executor x, functor on_success)
{
  // Retrieving URI from request, removing initial "/" from URI, before checking sanity of URI.
  auto uri = request()->envelope().uri().string();
  if (!sanity_check_uri (uri)) {

    // URI is not "sane".
    request()->write_error_response (x, 404);
    return;
  }

  // Retrieving full path of document, and ensuring it exists.
  path full_path = request()->envelope().path();
  if (!boost::filesystem::exists (full_path)) {

      // Writing error status response, and returning early.
      request()->write_error_response (x, 404);
      return;
  }

  // Returning file to client.
  write_head (full_path, x, on_success);
}


void head_handler::write_head (path full_path, exceptional_executor x, functor on_success)
{
  // First writing status 200.
  write_status (200, x, [this, x, full_path, on_success] (auto x) {

    // Notice, we are NOT writing any content in a HEAD response.
    // But we write entire response, including "Content-Length", and "Last-Modified", except the content parts.
    write_file_headers (full_path, true, x, on_success);
  });
}


} // namespace http_server
} // namespace rosetta
