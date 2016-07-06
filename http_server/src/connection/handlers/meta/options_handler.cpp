
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

#include "http_server/include/helpers/date.hpp"
#include "http_server/include/connection/request.hpp"
#include "http_server/include/connection/connection.hpp"
#include "http_server/include/connection/handlers/meta/options_handler.hpp"

namespace rosetta {
namespace http_server {

using std::string;
using namespace boost::asio;
using namespace boost::system;
using namespace rosetta::common;

string uri_encode (const string & entity);


options_handler::options_handler (connection_ptr connection, class request * request)
  : request_handler_base (connection, request)
{ }


void options_handler::handle (std::function<void()> on_success)
{/*
  // Writing status code.
  write_status (200, x, [this, x, on_success] (auto x) {

    // Building our request headers.
    collection headers {
      {"Content-Type", "text/plain; charset=utf-8" },
      {"Date", date::now ().to_string ()}};

    // Retrieving whether or not all possible verbs are allowed for resource.
    auto & auth = connection()->server()->authorization();
    auto ticket = request()->envelope().ticket();
    auto path = request()->envelope().path();
    bool trace = connection()->server()->configuration().get<bool> ("trace-allowed", false) && auth.authorize (ticket, path, "TRACE");
    bool head = connection()->server()->configuration().get<bool> ("head-allowed", false) && auth.authorize (ticket, path, "HEAD");
    bool get = auth.authorize (ticket, path, "GET");
    bool put = auth.authorize (ticket, path, "PUT") && (!exists (path) || auth.authorize (ticket, path, "DELETE"));
    bool del = auth.authorize (ticket, path, "DELETE");
    string allowed = "";
    if (del && put && get && head && trace) {

      // All verbs are allowed for resource.
      allowed = "*";
    } else {

      // Only some verbs are allowed for resource, OPTIONS is obviously one of them!
      allowed += "OPTIONS";
      if (trace)
        allowed += ", TRACE";
      if (head)
        allowed += ", HEAD";
      if (get)
        allowed += ", GET";
      if (put)
        allowed += ", PUT";
      if (del)
        allowed += ", DELETE";
    }

    // Adding "Allow" header to response.
    headers.push_back ( {"Allow", allowed} );

    // Writing HTTP headers to connection.
    write_headers (headers, x, [this, on_success] (auto x) {

      // Writing standard headers.
      write_standard_headers (x, [this, on_success] (auto x) {

        // Making sure we close envelope.
        ensure_envelope_finished (x, [this, on_success] (auto x) {

          // Invoking callback, signaling we're done.
          on_success (x);
        });
      });
    });
  });
*/}


} // namespace http_server
} // namespace rosetta
