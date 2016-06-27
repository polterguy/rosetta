
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

#include <tuple>
#include <vector>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "common/include/date.hpp"
#include "server/include/server.hpp"
#include "server/include/connection/request.hpp"
#include "server/include/connection/connection.hpp"
#include "server/include/exceptions/request_exception.hpp"
#include "server/include/connection/handlers/head_handler.hpp"

namespace rosetta {
namespace server {

using std::string;
using boost::system::error_code;
using namespace rosetta::common;


/// Verifies URI is sane, and not malformed, attempting to retrieve document outside of main folder, hidden files, etc.
bool sanity_check_uri (const string & uri);


head_handler::head_handler (class connection * connection, class request * request, const string & extension)
  : request_handler (connection, request),
    _extension (extension)
{ }


void head_handler::handle (exceptional_executor x, functor callback)
{
  // Retrieving URI from request, removing initial "/" from URI, before checking sanity of URI.
  string uri = request()->envelope().uri().substr (1);
  if (!sanity_check_uri (uri)) {

    // URI is not "sane".
    request()->write_error_response (x, 404);
    return;
  }

  // Retrieving root path, and building full path for document.
  const static string WWW_ROOT_PATH = connection()->server()->configuration().get<string> ("www-root", "www-root/");
  string full_path = WWW_ROOT_PATH + uri;

  // Making sure file exists.
  if (!boost::filesystem::exists (full_path)) {

      // Writing error status response, and returning early.
      request()->write_error_response (x, 404);
      return;
  }

  // Returning file to client.
  write_head (full_path, x, callback);
}


void head_handler::write_head (const string & full_path, exceptional_executor x, functor callback)
{
  // First writing status 200.
  write_status (200, x, [this, x, full_path, callback] (exceptional_executor x) {

    // Making sure we add the Last-Modified header for our file, to help clients and proxies cache the file.
    write_header ("Last-Modified", date::from_file_change (full_path).to_string (), x, [this, full_path, callback] (exceptional_executor x) {

      // Notice, we are NOT writing any content in a HEAD response.
      write_file_headers (full_path, true, x, callback);
    });
  });
}


} // namespace server
} // namespace rosetta
