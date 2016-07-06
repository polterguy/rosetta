
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

#include "http_server/include/connection/request.hpp"
#include "http_server/include/connection/connection.hpp"
#include "http_server/include/exceptions/server_exception.hpp"
#include "http_server/include/connection/handlers/meta/unauthorized_handler.hpp"

namespace rosetta {
namespace http_server {

using std::string;
using namespace rosetta::common;


unauthorized_handler::unauthorized_handler (class connection * connection, class request * request, bool allow_authentication)
  : error_handler (connection, request, 401),
    _allow_authentication (allow_authentication)
{ }


void unauthorized_handler::handle (std::function<void()> on_success)
{
  // Figuring out which file to serve.
  string error_file = "error-pages/401.html";

  // Checking if this is a 401 (Unauthorized), and if so, adding up the WWW-Authenticate header.
  // But only if caller says this is an OK thing to do, AND the configuration allows it to happen, if this request
  // is not secure.
  if (_allow_authentication && (connection()->is_secure() || connection()->server()->configuration().get<bool> ("authenticate-over-non-ssl", false))) {

    // Making sure we signal to client that it needs to authenticate.
    write_file (error_file, 401, {{"WWW-Authenticate", "Basic realm=\"User Visible Realm\""}}, [on_success] () {

      on_success ();
    });
  } else {

    // Using base class implementation for writing error file.
    write_file (error_file, 401, false, [on_success] () {

      on_success ();
    });
  }
}


} // namespace http_server
} // namespace rosetta
