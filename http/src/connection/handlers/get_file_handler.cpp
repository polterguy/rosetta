
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
#include "http/include/server.hpp"
#include "http/include/helpers/date.hpp"
#include "http/include/connection/request.hpp"
#include "http/include/connection/connection.hpp"
#include "http/include/exceptions/request_exception.hpp"
#include "http/include/connection/handlers/get_file_handler.hpp"

namespace rosetta {
namespace http {

using std::string;
using boost::system::error_code;
using namespace rosetta::common;


/// Verifies URI is sane, and not malformed, attempting to retrieve document outside of main folder, hidden files, etc.
bool sanity_check_uri (path uri);


get_file_handler::get_file_handler (class connection * connection, class request * request)
  : request_handler (connection, request)
{ }


void get_file_handler::handle (exceptional_executor x, functor on_success)
{
  // Retrieving URI from request, removing initial "/" from URI, before checking sanity of URI.
  auto uri = request()->envelope().uri();
  if (!sanity_check_uri (uri)) {

    // URI is not "sane".
    request()->write_error_response (x, 404);
    return;
  }

  // Retrieving root path, and making sure it exists.
  path full_path = request()->envelope().path();
  if (!boost::filesystem::exists (full_path)) {

      // Writing error status response, and returning early.
      request()->write_error_response (x, 404);
      return;
  }

  // Checking if we should render file, or a 304 (Not-Modified) response.
  if (should_write_file (full_path)) {

    // Returning file to client.
    write_file (full_path, 200, true, x, on_success);
  } else {

    // File has not been tampered with since the "If-Modified-Since" HTTP header, returning 304 response, without file content.
    write_304_response (x, on_success);
  }
}


bool get_file_handler::should_write_file (path full_path)
{
  // Checking if client passed in an "If-Modified-Since" header.
  string if_modified_since = request()->envelope().header ("If-Modified-Since");
  if (if_modified_since != "") {

    // We have an "If-Modified-Since" HTTP header, checking if file was tampered with since that date.
    date if_modified_date = date::parse (if_modified_since);
    date file_modify_date = date::from_file_change (full_path.string ());

    // Comparing dates.
    if (file_modify_date > if_modified_date) {

      // File was modified after "If-Modified-Since" header, hence we should write file to client.
      return true;
    } else {

      // File has not been tampered with since the "If-Modified-Since" HTTP header, hence we should not write file back to client.
      return false;
    }
  } else {

    // Client sent no "If-Modified-Since" header, hence we should write file back to client.
    return true;
  }
}


void get_file_handler::write_304_response (exceptional_executor x, functor on_success)
{
  // Writing status code 304 (Not-Modified) back to client.
  write_status (304, x, [this, on_success] (auto x) {

    // Writing standard HTTP headers to connection.
    write_standard_headers (x, [this, on_success] (auto x) {

      // Making sure we close envelope.      
      ensure_envelope_finished (x, [on_success] (auto x) {

        // invoking callback, since we're done writing the response.
        on_success (x);
      });
    });
  });
}


bool sanity_check_uri (path uri)
{
  // Breaking up URI into components, and sanity checking each component, to verify client is not requesting an illegal URI.
  for (auto & idx : uri) {

    if (idx.string().find ("..") != string::npos) // Request is probably trying to access files outside of the main "www-root" folder.
      return false;

    if (idx.string().find ("~") == 0) // Linux backup file or folder.
      return false;

    if (idx.string().find (".") == 0) // Linux hidden file or folder, or a file without a name, and only extension.
      return false;
  }
  return true; // URI is sane.
}


} // namespace http
} // namespace rosetta
