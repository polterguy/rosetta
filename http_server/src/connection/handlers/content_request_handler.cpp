
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

#include "http_server/include/server.hpp"
#include "http_server/include/connection/request.hpp"
#include "http_server/include/exceptions/request_exception.hpp"
#include "http_server/include/connection/connection.hpp"
#include "http_server/include/connection/handlers/content_request_handler.hpp"

namespace rosetta {
namespace http_server {

using std::string;


content_request_handler::content_request_handler (class request * request)
  : request_handler_base (request)
{ }


size_t content_request_handler::get_content_length (connection_ptr connection)
{
  // Max allowed length of content.
  const size_t MAX_REQUEST_CONTENT_LENGTH = connection->server()->configuration().get<size_t> ("max-request-content-length", 4194304);

  // Checking if there is any content first.
  string content_length_str = request()->envelope().header ("Content-Length");

  // Checking if there is any Content-Length
  if (content_length_str.size() == 0) {

    // No content.
    return 0;
  } else {

    // Checking that Content-Length does not exceed max request content length.
    auto content_length = boost::lexical_cast<size_t> (content_length_str);
    if (content_length > MAX_REQUEST_CONTENT_LENGTH)
      throw request_exception ("Request content was too long.");

    // Returning Content-Length to caller.
    return content_length;
  }
}


} // namespace http_server
} // namespace rosetta
