
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
#include "http_server/include/helpers/uri_encode.hpp"
#include "http_server/include/connection/request.hpp"
#include "http_server/include/connection/connection.hpp"
#include "http_server/include/exceptions/request_exception.hpp"
#include "http_server/include/connection/handlers/post_authorization_handler.hpp"

namespace rosetta {
namespace http_server {

using std::string;
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
  // Finding out which verb this request wants to change the value of.
  auto verb_iter = std::find_if (_parameters.begin(), _parameters.end(), [] (auto & idx) {
    return std::get<0> (idx) == "verb";
  });
  if (verb_iter == _parameters.end ())
    throw request_exception ("Unrecognized HTTP POST request, missing 'verb' parameter."); // Not recognized, hence a "bug".
  string verb = std::get<1> (*verb_iter);

  // Finding out the new value of the verb.
  auto value_iter = std::find_if (_parameters.begin(), _parameters.end(), [] (auto & idx) {
    return std::get<0> (idx) == "value";
  });

  // Retrieving the action client wants to perform.
  if (value_iter == _parameters.end ())
    throw request_exception ("Unrecognized HTTP POST request, missing 'value' parameter."); // Not recognized, hence a "bug".
  string value = std::get<1> (*value_iter);

  // Updating authorization file for current path.
  connection()->server()->authorization().update (request()->envelope().path(), verb, value, [x, on_success] (bool success) {

    // Invoking callback.
    on_success (x);
  });
}


} // namespace http_server
} // namespace rosetta
