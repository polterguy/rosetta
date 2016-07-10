
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
#include "http_server/include/connection/handlers/post_handler_base.hpp"

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


post_handler_base::post_handler_base (class request * request)
  : content_request_handler (request)
{ }


void post_handler_base::handle (connection_ptr connection, std::function<void()> on_success)
{
  // Setting deadline timer for content read.
  const int POST_CONTENT_READ_TIMEOUT = connection->server()->configuration().get<int> ("request-post-content-read-timeout", 30);
  connection->set_deadline_timer (POST_CONTENT_READ_TIMEOUT);

  // Reading content of request.
  auto content_length = get_content_length(connection);
  if (content_length == 0) {

    // Not acceptable.
    request()->write_error_response (connection, 500);
  } else {

    // Retrieving content from socket.
    connection->socket().async_read (connection->buffer(),
                                     transfer_exactly_t (content_length),
                                     [this, connection, on_success] (auto error, auto bytes_read) {

      // Checking that no socket errors occurred.
      if (error) {

        // Something went wrong.
        connection->close();
      } else {

        // Making sure connection is closed, in case an exception occurs.
        exceptional_executor x ([connection] () {connection->close ();});

        // Reading content into stream;
        istream stream (&connection->buffer());
        shared_ptr<unsigned char> buffer (new unsigned char [bytes_read], std::default_delete<unsigned char []>());
        stream.read (reinterpret_cast<char*> (buffer.get()), bytes_read);

        // Creating a string out of content, before splitting it into each name/value pair.
        string str_content (buffer.get(), buffer.get () + bytes_read);
        vector<string> parameters;
        split (parameters, str_content, boost::is_any_of ("&"));
        for (auto & idx : parameters) {
          vector<string> name_value;
          split (name_value, idx, boost::is_any_of ("="));

          // Making sure there is both a name and a value in current parameter.
          if (name_value.size() != 2)
            throw request_exception ("Bad data found in POST request.");

          // Retrieving name and value.
          string name  = uri_encode::decode (name_value [0]);
          string value = uri_encode::decode (name_value [1]);

          // Sanitizing both name and value.
          for (auto idx : name) {
            if (idx < 32 || idx > 126)
              throw request_exception ("Bad characters found in POST request content.");
          }
          for (auto idx : value) {
            if (idx < 32 || idx > 126)
              throw request_exception ("Bad characters found in POST request content.");
          }
          _parameters.push_back ( {name, value} );
        }

        // Evaluates request, now that we have the data supplied by client.
        on_success ();

        // Releasing exception handler.
        x.release();
      }
    });
  }
}


} // namespace http_server
} // namespace rosetta
