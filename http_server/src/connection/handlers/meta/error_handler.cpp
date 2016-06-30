
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
#include "http_server/include/connection/handlers/meta/error_handler.hpp"

namespace rosetta {
namespace http_server {

using std::string;
using namespace rosetta::common;


error_handler::error_handler (class connection * connection, class request * request, unsigned int status_code)
  : request_file_handler (connection, request),
    _status_code (status_code)
{
  // Verify that this actually is an error, and if not, throws an exception.
  if (_status_code < 400)
    throw server_exception ("Logical error in server. Tried to return a non-error status code as an error to client.");
}


void error_handler::handle (exceptional_executor x, functor on_success)
{
  // Figuring out which file to serve.
  string error_file = "error-pages/" + boost::lexical_cast<string> (_status_code) + ".html";

  // Using base class implementation for writing error file.
  write_file (error_file, _status_code, false, x, [on_success] (exceptional_executor x) {

    // Intentionally letting x go out of scope, without invoking on_success(), to close connection.
  });
}


} // namespace http_server
} // namespace rosetta
