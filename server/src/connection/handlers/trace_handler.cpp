
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

#include <vector>
#include "common/include/date.hpp"
#include "server/include/connection/request.hpp"
#include "server/include/connection/connection.hpp"
#include "server/include/connection/handlers/trace_handler.hpp"

namespace rosetta {
namespace server {

using std::string;
using namespace boost::asio;
using namespace boost::system;
using namespace rosetta::common;

string uri_encode (const string & entity);


trace_handler::trace_handler (class connection * connection, class request * request)
  : request_handler (connection, request)
{ }


void trace_handler::handle (exceptional_executor x, functor callback)
{
  // Writing status code.
  write_status (200, x, [this, x, callback] (exceptional_executor x) {

    // Figuring out what we're sending, before we send the headers, to see the size of our request.
    auto buffer_ptr = build_content ();

    // Building our request headers.
    collection headers {
      {"Content-Type", "text/plain; charset=utf-8" },
      {"Date", date::now ().to_string ()},
      {"Content-Length", boost::lexical_cast<string> (buffer_ptr->size ())}};

    // Writing HTTP headers to connection.
    write_headers (headers, x, [this, callback, buffer_ptr] (exceptional_executor x) {

      // Writing standard headers.
      write_standard_headers (x, [this, callback, buffer_ptr] (auto x) {

        // Making sure we close envelope.
        ensure_envelope_finished (x, [this, callback, buffer_ptr] (auto x) {

          // Writing entire request, HTTP-Request line, and HTTP headers, back to client, as content.
          connection()->socket().async_write (buffer (*buffer_ptr), [buffer_ptr, callback, x] (const error_code & error, size_t bytes_written) {

            // Invoking callback, signaling we're done.
            callback (x);
          });
        });
      });
    });
  });
}


std::shared_ptr<std::vector<unsigned char> > trace_handler::build_content ()
{
  std::shared_ptr<std::vector<unsigned char> > buffer_ptr =  std::make_shared<std::vector<unsigned char> >();

  // Return the HTTP-Request line.
  // Starting with HTTP method.
  buffer_ptr->insert (buffer_ptr->end(), request()->envelope().type ().begin(), request()->envelope().type().end());
  buffer_ptr->push_back (' ');

  // Then the URI, without the parameters.
  buffer_ptr->insert (buffer_ptr->end(), request()->envelope().uri ().begin(), request()->envelope().uri().end());

  // Pushing parameters into the HTTP-Request line URI.
  bool first = true;
  for (auto idx : request()->envelope().parameters()) {

    // Checking if this is the first parameter, or consecutive ones, to append either '?' or '&' accordingly.
    if (first) {
      first = false;
      buffer_ptr->push_back ('?');
    } else {
      buffer_ptr->push_back ('&');
    }

    // Adding name of parameter, making sure we URI encode it.
    const auto name = uri_encode (std::get<0> (idx));
    buffer_ptr->insert (buffer_ptr->end(), name.begin(), name.end());

    // Adding value of parameter, making sure we URI encode it.
    const auto value = uri_encode (std::get<1> (idx));
    if (value.size() > 0) {

      // We only add '=' and value, if there actually is any value.
      buffer_ptr->push_back ('=');
      buffer_ptr->insert (buffer_ptr->end(), value.begin(), value.end());
    }
  }

  // Adding HTTP version into content buffer.
  buffer_ptr->push_back (' ');
  buffer_ptr->insert (buffer_ptr->end(), request()->envelope().version().begin(), request()->envelope().version().end());

  // CR/LF sequence, to prepare for HTTP headers.
  buffer_ptr->push_back ('\r');
  buffer_ptr->push_back ('\n');

  // Returning all HTTP headers.
  for (auto idx : request()->envelope().headers ()) {

    // Header name and colon.
    buffer_ptr->insert (buffer_ptr->end(), std::get<0> (idx).begin(), std::get<0> (idx).end());
    buffer_ptr->push_back (':');

    // Header value, prepended by a SP, for then to finish header with CR/LF sequence.
    buffer_ptr->push_back (' ');
    buffer_ptr->insert (buffer_ptr->end(), std::get<1> (idx).begin(), std::get<1> (idx).end());
    buffer_ptr->push_back ('\r');
    buffer_ptr->push_back ('\n');
  }

  // Returning result to caller.
  return buffer_ptr;
}


/// Transforms the given unsigned char, which should be between [0-16), to a hex character.
unsigned char to_hex (unsigned char ch)
{
  if (ch >= 0 && ch < 10)
    ch += '0';
  else if (ch >= 10 && ch < 16)
    ch += ('a' -10);
  return ch;
}


string uri_encode (const string & entity)
{
  std::vector<unsigned char> return_value;
  for (unsigned char idx : entity) {
    if (idx == ' ') {

      // Encoding as '+'.
      return_value.push_back ('+');
    } else if ((idx >= 'a' && idx <= 'z') ||
               (idx >= 'A' && idx <= 'Z') ||
               (idx >= '0' && idx <= '9') ||
               idx == '-' || idx == '_' || idx == '.' || idx == '~') {

      // No need to encode.
      return_value.push_back (idx);
    } else {

      // Encoding with % syntax.
      return_value.push_back ('%');
      return_value.push_back (to_hex (idx >> 4));
      return_value.push_back (to_hex (idx & 0x0f));
    }
  }
  return string (return_value.begin (), return_value.end ());
}


} // namespace server
} // namespace rosetta
