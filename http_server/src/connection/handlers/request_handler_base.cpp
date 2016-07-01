
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

#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include "http_server/include/server.hpp"
#include "http_server/include/helpers/date.hpp"
#include "http_server/include/connection/request.hpp"
#include "http_server/include/connection/connection.hpp"
#include "http_server/include/exceptions/request_exception.hpp"
#include "http_server/include/connection/handlers/request_handler_base.hpp"

using std::string;
using std::shared_ptr;
using std::make_shared;
using namespace boost::asio;
using namespace rosetta::common;

namespace rosetta {
namespace http_server {


request_handler_base::request_handler_base (class connection * connection, class request * request)
  : _connection (connection),
    _request (request)
{ }


void request_handler_base::write_status (unsigned int status_code, exceptional_executor x, functor on_success)
{
  // Creating status line, and serializing to socket, making sure status_line stays around until after write operation is finished.
  shared_ptr<string> status_line = make_shared<string> ("HTTP/1.1 " + boost::lexical_cast<string> (status_code) + " ");
  switch (status_code) {
  case 200:
    *status_line += "OK";
    break;
  case 304:
    *status_line += "Not Modified";
    break;
  case 307:
    *status_line += "Moved Temporarily";
    break;
  case 401:
    *status_line += "Unauthorized";
    break;
  case 403:
    *status_line += "Forbidden";
    break;
  case 404:
    *status_line += "Not Found";
    break;
  case 405:
    *status_line += "Method Not Allowed";
    break;
  case 413:
    *status_line += "Request Header Too Long";
    break;
  case 414:
    *status_line += "Request-URI Too Long";
    break;
  case 500:
    *status_line += "Internal Server Error";
    break;
  case 501:
    *status_line += "Not Implemented";
    break;
  default:
    if (status_code > 200 && status_code < 300) {

      // Some sort of unknown success status.
      *status_line += "Unknown Success Type";
    } else if (status_code >= 300 && status_code < 400) {

      // Some sort of unknown redirection status.
      *status_line += "Unknown Redirection Type";
    } else {

      // Some sort of unknown error type.
      *status_line += "Unknown Error Type";
    } break;
  }
  *status_line += "\r\n";

  // Writing status line to socket.
  _connection->socket().async_write (buffer (*status_line), [on_success, x, status_line] (auto error, auto bytes_written) {

    // Sanity check.
    if (error)
      throw request_exception ("Socket error while writing HTTP status line.");
    else
      on_success (x);
  });
}


void request_handler_base::write_headers (collection headers, exceptional_executor x, functor on_success)
{
  if (headers.size() == 0) {

    // No more headers, invoking on_success().
    on_success (x);

  } else {

    // Retrieving next key/value pair.
    string key = std::get<0> (headers [0]);
    string value = std::get<1> (headers [0]);

    // Popping off currently written header from list of headers.
    headers.erase (headers.begin (), headers.begin () + 1);

    // Writing header.
    write_header (key, value, x, [this, headers, on_success] (auto x) {

      // Invoking self, having popped off the first header in the collection.
      write_headers (headers, x, on_success);
    });
  }
}


void request_handler_base::write_header (const string & key, const string & value, exceptional_executor x, functor on_success)
{
  // Creating header, making sure the string stays around until after socket write operation is finished.
  shared_ptr<string> header_content = make_shared<string> (key + ": " + value + "\r\n");

  // Writing header content to socket.
  _connection->socket().async_write (buffer (*header_content), [this, on_success, x, header_content] (auto error, auto bytes_written) {

    // Sanity check.
    if (error)
      throw request_exception ("Socket error while writing HTTP header.");
    else
      on_success (x);
  });
}


void request_handler_base::write_standard_headers (exceptional_executor x, functor on_success)
{
  // Making things more tidy in here.
  using namespace std;
  using namespace boost::algorithm;

  // Creating header, making sure the string stays around until after socket write operation is finished.
  // First we add up the "Date" header, which should be returned with every single request, regardless of its type.
  shared_ptr<string> header_content = make_shared<string> ("Date: " + date::now ().to_string () + "\r\n");

  // Making sure we submit the server name back to client, if server is configured to do this.
  // Notice, even if server configuration says that server should identify itself, we do not provide any version information!
  // This is to make it harder to create a "targeted attack" trying to hack the server.
  if (_connection->server()->configuration().get<bool> ("provide-server-info", false))
    *header_content += "Server: Rosetta\r\n";

  // Checking if server is configured to render "static headers".
  // Notice that static headers are defined as a pipe separated (|) list of strings, with both name and value of header, for instance
  // "Foo: bar|Howdy-World: circus". The given example would render two static headers, "Foo" and "Howdy-World", with their respective values.
  const string static_headers = _connection->server()->configuration().get <string> ("static-response-headers", "");
  if (static_headers.size() > 0) {

    // Server is configured to render "static headers".
    // Making sure we append the "static headers" from configuration into response.
    vector<string> headers;
    split (headers, static_headers, boost::is_any_of ("|"));
    for (auto & idx : headers) {
      *header_content += idx + "\r\n";
    }
  }

  // Writing header content to socket.
  _connection->socket().async_write (buffer (*header_content), [on_success, x, header_content] (auto error, auto bytes_written) {

    // Sanity check.
    if (error)
      throw request_exception ("Socket error while writing HTTP header.");
    else
      on_success (x);
  });
}


void request_handler_base::ensure_envelope_finished (exceptional_executor x, functor on_success)
{
  // Creating last empty line, to finish of envelope, making sure our buffer stays around, until async_write is finished doing its thing.
  shared_ptr<string> cr_lf = make_shared<string> ("\r\n");

  // Writing header content to socket.
  _connection->socket().async_write (buffer (*cr_lf), [on_success, x, cr_lf] (auto error, auto bytes_written) {

    // Sanity check.
    if (error)
      throw request_exception ("Socket error while writing HTTP header.");
    else
      on_success (x);
  });
}


string request_handler_base::get_mime (path filename)
{
  // Then we do a lookup into the configuration for our server, to see if it has defined a MIME type for the given file's extension.
  string mime_type = connection()->server()->configuration().get<string> ("mime" + filename.extension().string (), "");

  // Returning MIME type to caller.
  return mime_type;
}


} // namespace http_server
} // namespace rosetta
