
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

#include <memory>
#include <istream>
#include <boost/algorithm/string.hpp>
#include "http_server/include/server.hpp"
#include "http_server/include/helpers/uri_encode.hpp"
#include "http_server/include/connection/request.hpp"
#include "http_server/include/connection/connection.hpp"
#include "http_server/include/exceptions/request_exception.hpp"
#include "http_server/include/connection/handlers/post_authorization_handler.hpp"

namespace rosetta {
namespace http_server {

using std::string;
using std::vector;
using std::istream;
using std::shared_ptr;
using std::default_delete;
using namespace boost::algorithm;
using namespace boost::asio::detail;
using namespace rosetta::common;


post_authorization_handler::post_authorization_handler (class connection * connection, class request * request)
  : post_handler_base (connection, request)
{ }


void post_authorization_handler::handle (exceptional_executor x, functor on_success)
{
  // Letting base class do the heavy lifting.
  post_handler_base::handle (x, [this, on_success] (auto x) {

    // Evaluates request, now that we have the data supplied by client.
    evaluate (x, on_success);
  });
}


void post_authorization_handler::evaluate (exceptional_executor x, functor on_success)
{
  // Finding out which action this request wants to perform.
  auto action_iter = std::find_if (_parameters.begin(), _parameters.end(), [] (auto & idx) {
    return std::get<0> (idx) == "action";
  });

  // Retrieving the action client wants to perform.
  if (action_iter == _parameters.end ())
    throw request_exception ("Unrecognized HTTP POST request, missing 'action' parameter."); // Not recognized, hence a "bug".
  string action = std::get<1> (*action_iter);

  // Checking if client is authenticated as root, which has extended privileges.
  if (request()->envelope().ticket().role == "root") {

    // We're fine!
  } else {

    // Only authenticated clients are allowed to do anything here!
    throw request_exception ("Non-authenticated client tried to POST.");
  }
}


} // namespace http_server
} // namespace rosetta
