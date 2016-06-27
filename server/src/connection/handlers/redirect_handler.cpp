
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
#include "server/include/connection/handlers/redirect_handler.hpp"

namespace rosetta {
namespace server {

using std::string;
using boost::system::error_code;
using namespace rosetta::common;


redirect_handler::redirect_handler (class connection * connection, class request * request, unsigned int status, const string & uri, bool no_store)
  : request_handler (connection, request),
    _status (status),
    _uri (uri),
    _no_store (no_store)
{ }


void redirect_handler::handle (exceptional_executor x, functor callback)
{
  // First writing status.
  write_status (_status, x, [this, x, callback] (exceptional_executor x) {

    // Then writing "Location" of resource requested.
    collection list = {{"Location", _uri}};
    if (_no_store) {

      // This redirect request should not be cached.
      list.push_back (std::tuple<string, string> ("Cache-Control", "no-store"));
    }

    // Rendering the headers, Location, and possibly Cache-Control.
    write_headers (list, x, [this, callback] (exceptional_executor x) {

      // Then making sure we add the "standard headers".
      write_standard_headers (x, [this, callback] (auto x) {

        // Then making sure we close our response envelope.
        ensure_envelope_finished (x, [callback] (auto x) {

          // Notice, we are NOT writing any content in a redirect response.
          // Hence we invoke the on_success() callback provided when request was created.
          callback (x);        
        });
      });
    });
  });
}


} // namespace server
} // namespace rosetta
