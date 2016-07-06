
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
#include "http_server/include/server.hpp"
#include "http_server/include/helpers/date.hpp"
#include "http_server/include/connection/request.hpp"
#include "http_server/include/connection/connection.hpp"
#include "http_server/include/exceptions/request_exception.hpp"
#include "http_server/include/connection/handlers/get_file_handler.hpp"

namespace rosetta {
namespace http_server {

using std::string;
using boost::system::error_code;
using namespace rosetta::common;


get_file_handler::get_file_handler (class connection * connection, class request * request)
  : request_file_handler (connection, request)
{ }


void get_file_handler::handle (std::function<void()> on_success)
{
  // Retrieving root path, and checking if we should write it.
  path full_path = request()->envelope().path();
  if (should_write_file (full_path)) {

    // Returning file to client.
    write_file (full_path, 200, true, on_success);
  } else {

    // File has not been tampered with since the "If-Modified-Since" HTTP header, returning 304 response, without file content.
    write_304_response (on_success);
  }
}


bool get_file_handler::should_write_file (path full_path)
{
  // Checking if client passed in an "If-Modified-Since" header.
  string if_modified_since = request()->envelope().header ("If-Modified-Since");
  if (if_modified_since != "") {

    // We have an "If-Modified-Since" HTTP header, checking if file was tampered with since that date.
    date if_modified_date = date::parse (if_modified_since);
    date file_modify_date = date::from_path_change (full_path);

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


void get_file_handler::write_304_response (std::function<void()> on_success)
{
  // Writing status code 304 (Not-Modified) back to client.
  write_status (304, [this, on_success] () {

    // Writing standard HTTP headers to connection.
    write_standard_headers ([this, on_success] () {

      // Making sure we close envelope.      
      ensure_envelope_finished ([on_success] () {

        // invoking callback, since we're done writing the response.
        on_success ();
      });
    });
  });
}


} // namespace http_server
} // namespace rosetta
