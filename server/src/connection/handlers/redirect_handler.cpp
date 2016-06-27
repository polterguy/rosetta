
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

#include "server/include/connection/request.hpp"
#include "server/include/connection/connection.hpp"
#include "server/include/connection/handlers/redirect_handler.hpp"

namespace rosetta {
namespace server {

using std::string;
using boost::system::error_code;
using namespace rosetta::common;


redirect_handler::redirect_handler (class connection * connection,
                                    class request * request,
                                    unsigned int status,
                                    const string & uri,
                                    bool no_store)
  : request_handler (connection, request),
    _status (status),
    _uri (uri),
    _no_store (no_store)
{ }


void redirect_handler::handle (exceptional_executor x, functor on_success)
{
  // First writing status.
  write_status (_status, x, [this, x, on_success] (auto x) {

    // Then writing "Location" of resource requested.
    collection list = {{"Location", _uri}};

    // Checking if this request should be cached or not.
    if (_no_store)
      list.push_back ({"Cache-Control", "no-store"});

    // Rendering the headers, "Location", and possibly "Cache-Control".
    write_headers (list, x, [this, on_success] (auto x) {

      // Then making sure we add the "standard headers".
      write_standard_headers (x, [this, on_success] (auto x) {

        // Then making sure we close our response envelope.
        ensure_envelope_finished (x, [on_success] (auto x) {

          // Finished handling request.
          on_success (x);        
        });
      });
    });
  });
}


} // namespace server
} // namespace rosetta
