
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


head_handler::head_handler (class request * request)
  : request_file_handler (request)
{ }


void head_handler::handle (connection_ptr connection, std::function<void()> on_success)
{
  // First writing status 200.
  write_status (connection, 200, [this, connection, on_success] () {

    // Notice, we are NOT writing any content in a HEAD response.
    // But we write entire response, including "Content-Length", and "Last-Modified", except the content parts.
    write_file_headers (connection, request()->envelope().path(), true, on_success);
  });
}


} // namespace http_server
} // namespace rosetta
