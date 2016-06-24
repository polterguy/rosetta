
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
#include "server/include/connection/handlers/static_file_handler.hpp"

namespace rosetta {
namespace server {

using std::string;
using boost::system::error_code;
using namespace rosetta::common;


/// Verifies URI is sane, and not malformed, attempting to retrieve document outside of main folder, hidden files, etc.
bool sanity_check_uri (const string & uri);


static_file_handler::static_file_handler (class connection * connection, class request * request, const string & extension)
  : request_handler (connection, request),
    _extension (extension)
{ }


void static_file_handler::handle (exceptional_executor x, functor callback)
{
  // Retrieving URI from request, removing initial "/" from URI, before checking sanity of URI.
  string uri = request()->envelope().get_uri().substr (1);
  if (!sanity_check_uri (uri)) {

    // URI is not "sane".
    request()->write_error_response (x, 404);
    return;
  }

  // Retrieving root path.
  const static string WWW_ROOT_PATH = connection()->server()->configuration().get<string> ("www-root", "www-root/");
  
  // Figuring out entire physical path of file.
  string full_path = WWW_ROOT_PATH + uri;

  // Making sure file exists.
  if (!boost::filesystem::exists (full_path)) {

      // Writing error status response, and returning early.
      request()->write_error_response (x, 404);
      return;
  }

  // Checking if client passed in an "If-Modified-Since" header, and if so, handle it accordingly.
  string if_modified_since = request()->envelope().get_header("If-Modified-Since");
  if (if_modified_since != "") {

    // We have an "If-Modified-Since" HTTP header, checking if file was tampered with since that date.
    date if_modified_date = date::parse (if_modified_since);
    date file_modify_date = date::from_file_change (full_path);

    // Comparing dates.
    if (file_modify_date > if_modified_date) {

      // File has been tampered with since the "If-Modified-Since" HTTP header, returning file in response.
      // First writing status 200.
      write_status (200, x, [this, x, full_path, callback] (exceptional_executor x) {

        // Making sure we add the Last-Modified header for our file, to help clients and proxies cache the file.
        write_header ("Last-Modified", date::from_file_change (full_path).to_string (), x, [this, full_path, callback] (exceptional_executor x) {

          // Then writing actual file.
          write_file (full_path, x, callback);
        });
      });
    } else {

      // File has not been tampered with since the "If-Modified-Since" HTTP header, returning 304 response, without file content.
      write_status (304, x, [this, callback] (exceptional_executor x) {

        // Building our request headers.
        std::vector<std::tuple<string, string> > headers { {"Date", date::now ().to_string ()} };

        // Writing HTTP headers to connection.
        write_headers (headers, x, [this, callback] (exceptional_executor x) {

          // invoking callback, since we're done writing the response.
          callback (x);
        }, true);
      });
    }
  } else {

    // First writing status 200.
    write_status (200, x, [this, x, full_path, callback] (exceptional_executor x) {

      // Making sure we add the Last-Modified header for our file, to help clients and proxies cache the file.
      write_header ("Last-Modified", date::from_file_change (full_path).to_string (), x, [this, full_path, callback] (exceptional_executor x) {

        // Then writing actual file.
        write_file (full_path, x, callback);
      });
    });
  }
}


bool sanity_check_uri (const string & uri)
{
  // Breaking up URI into components, and sanity checking each component, to verify client is not requesting an illegal URI.
  std::vector<string> entities;
  boost::split (entities, uri, boost::is_any_of ("/"));
  for (string & idx : entities) {

    if (idx == "")
      return false; // Two consecutive "/" after each other.

    if (idx.find ("..") != string::npos) // Request is probably trying to access files outside of the main www-root folder.
      return false;

    if (idx.find ("~") == 0) // Linux backup file or folder.
      return false;

    if (idx.find (".") == 0) // Linux hidden file or folder, or a file without a name, and only extension.
      return false;
  }
  return true; // URI is sane.
}


} // namespace server
} // namespace rosetta
