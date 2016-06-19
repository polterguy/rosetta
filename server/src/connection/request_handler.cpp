
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
#include "server/include/server.hpp"
#include "server/include/connection/request.hpp"
#include "server/include/connection/connection.hpp"
#include "server/include/connection/request_handler.hpp"
#include "server/include/exceptions/request_exception.hpp"
#include "server/include/connection/static_file_handler.hpp"

namespace rosetta {
namespace server {
  
using std::get;
using std::string;
using boost::system::error_code;
using boost::asio::buffer;
using boost::asio::async_write;


request_handler_ptr request_handler::create (server * server, connection * connection, request * request, const string & extension)
{
  // Figuring out which handler to use, according to configuration of server.
  auto handler = server->configuration().get<string> (extension + "-handler", server->configuration().get<string> ("default-handler"));

  // Returning the correct handler to caller.
  if (handler == "static-file-handler")
    return request_handler_ptr (new static_file_handler (server, connection, request));

  throw request_exception ("Unknown handler type for files with extension; '" + extension + "', handler name was; '" + handler + "'");
}


request_handler::request_handler (server * server, connection * connection, request * request)
  : _server (server),
    _connection (connection),
    _request (request)
{ }


void request_handler::write_status (unsigned int status_code, exceptional_executor x, function<void (exceptional_executor x)> callback)
{
  // Creating status line, and serializing to socket.
  string status_line = "HTTP/1.1 " + lexical_cast<string> (status_code) + " ";
  switch (status_code) {
  case 200:
    status_line += "OK";
    break;
  case 304:
    status_line += "Not Modified";
    break;
  case 404:
    status_line += "Not Found";
    break;
  case 405:
    status_line += "Method Not Allowed";
    break;
  case 413:
    status_line += "Request Header Too Long";
    break;
  case 414:
    status_line += "Request-URI Too Long";
    break;
  case 501:
    status_line += "Not Implemented";
    break;
  default:
    if (status_code > 200 && status_code < 300)
      status_line += "Unknown Success Type";
    else if (status_code >= 300 && status_code < 400)
      status_line += "Unknown Type";
    else
      status_line += "Unknown Error Type";
    break;
  }
  status_line += "\r\n";

  // Writing status line to socket.
  async_write (*_connection->socket(), buffer (status_line), [callback, x] (const error_code & error, size_t bytes_written) {

    // Sanity check.
    if (error)
      throw request_exception ("Socket error while returning HTTP status line back to client.");
    else
      callback (x);
  });
}


void request_handler::write_header (const string & key, const string & value, exceptional_executor x, function<void (exceptional_executor x)> callback)
{
  // Writing HTTP header on socket.
  async_write (*_connection->socket(), buffer (key + ": " + value + "\r\n"), [callback, x] (const error_code & error, size_t bytes_written) {

    // Sanity check.
    if (error)
      throw request_exception ("Socket error while returning HTTP header bacl to client.");
    else if (callback != nullptr)
      callback (x);
  });
}


void request_handler::write_headers (vector<tuple<string, string> > headers, exceptional_executor x, function<void (exceptional_executor x)> callback)
{
  if (headers.size() == 0) {

    // No more headers.
    if (callback != nullptr)
      callback (x);
  } else {

    // Retrieving next key/value pair.
    string key = get<0> (headers [0]);
    string value = get<1> (headers [0]);

    // Popping off currently written header from list of headers.
    headers.erase (headers.begin (), headers.begin () + 1);

    // Writing header.
    write_header (key, value, x, [this, headers, callback] (exceptional_executor x) {

      // Invoking self.
      write_headers (headers, x, callback);
    });
  }
}


} // namespace server
} // namespace rosetta
