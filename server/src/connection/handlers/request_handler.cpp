
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
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "common/include/date.hpp"
#include "server/include/server.hpp"
#include "server/include/connection/request.hpp"
#include "server/include/connection/connection.hpp"
#include "server/include/exceptions/request_exception.hpp"
#include "server/include/connection/handlers/error_handler.hpp"
#include "server/include/connection/handlers/request_handler.hpp"
#include "server/include/connection/handlers/static_file_handler.hpp"

namespace rosetta {
namespace server {

using std::string;
using boost::system::error_code;
using namespace rosetta::common;


request_handler_ptr request_handler::create (connection * connection, request * request, int status_code)
{
  if (status_code >= 400) {

    // Some sort of error.
    return request_handler_ptr (new error_handler (connection, request, status_code));

  } else {

    // Figuring out handler to use according to request extension.
    const string & extension = request->envelope().get_extension();
    string handler = extension.size () == 0 ?
        connection->server()->configuration().get<string> ("default-handler", "error") :
        connection->server()->configuration().get<string> (extension + "-handler", "error");

    // Returning the correct handler to caller.
    if (handler == "static-file-handler")
      return request_handler_ptr (new static_file_handler (connection, request, extension));
    else
      return request_handler_ptr (new error_handler (connection, request, 404));
  }
}


request_handler::request_handler (connection * connection, request * request)
  : _connection (connection),
    _request (request)
{ }


void request_handler::write_status (unsigned int status_code, exceptional_executor x, std::function<void (exceptional_executor x)> functor)
{
  // Creating status line, and serializing to socket.
  string status_line = "HTTP/1.1 " + boost::lexical_cast<string> (status_code) + " ";
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
  async_write (_connection->socket(), boost::asio::buffer (status_line), [functor, x] (const error_code & error, size_t bytes_written) {

    // Sanity check.
    if (error)
      return; // Simply let x go out of scope should clean things up.
    else
      functor (x);
  });
}


void request_handler::write_header (const string & key, const string & value, exceptional_executor x, std::function<void (exceptional_executor x)> functor)
{
  // Writing HTTP header on socket.
  async_write (_connection->socket(), boost::asio::buffer (key + ":" + value + "\r\n"), [functor, x] (const error_code & error, size_t bytes_written) {

    // Sanity check.
    if (error)
      return; // Simply letting x go out of scope, should clean things up.
    else
      functor (x);
  });
}


void request_handler::write_headers (std::vector<std::tuple<string, string> > headers, exceptional_executor x, std::function<void (exceptional_executor x)> functor)
{
  if (headers.size() == 0) {

    // No more headers.
    if (functor != nullptr)
      functor (x);
  } else {

    // Retrieving next key/value pair.
    string key = std::get<0> (headers [0]);
    string value = std::get<1> (headers [0]);

    // Popping off currently written header from list of headers.
    headers.erase (headers.begin (), headers.begin () + 1);

    // Writing header.
    write_header (key, value, x, [this, headers, functor] (exceptional_executor x) {

      // Invoking self.
      write_headers (headers, x, functor);
    });
  }
}


void request_handler::write_file (const string & filepath, exceptional_executor x, std::function<void (exceptional_executor x)> functor)
{
  // Figuring out size of file, and making sure it's not larger than what we are allowed to handle according to configuration of server.
  size_t size = boost::filesystem::file_size (filepath);

  // Retrieving MIME type, and verifying this is a type of file we actually serve.
  string mime_type = get_mime (filepath);
  if (mime_type == "") {

    // File type is not served according to configuration of server.
    _request->write_error_response (x, 403);
    return;
  }

  // Building our request headers.
  std::vector<std::tuple<string, string> > headers {
    {"Content-Type", mime_type },
    {"Date", date::now ().to_string ()},
    {"Content-Length", boost::lexical_cast<string> (size)}};

  // Writing HTTP headers to connection.
  write_headers (headers, x, [this, filepath, functor] (exceptional_executor x) {

    // Writing additional CR/LF sequence, to signal to client that we're beginning to send content.
    async_write (_connection->socket(), boost::asio::buffer (string("\r\n")), [this, filepath, functor, x] (const error_code & error, size_t bytes_written) {

      // Sanity check.
      if (error)
        return; // Simply let x go out of scope to clean things up.

      // Reading file's content, and putting it into a vector.
      std::ifstream fs (filepath, std::ios_base::binary);
      std::vector<char> file_content ((std::istreambuf_iterator<char> (fs)), std::istreambuf_iterator<char>());

      // Writing content to connection's socket.
      async_write (_connection->socket(), boost::asio::buffer (file_content), [functor, x] (const error_code & error, size_t bytes_written) {

        // Sanity check.
        if (error)
          return; // Letting x go out of scope to clean things up.

        // Finished serving static file, invoking callback supplied when invoking method.
        functor (x);
      });
    });
  });
}


string request_handler::get_mime (const string & filepath)
{
  // Returning MIME type for file extension.
  string filename = filepath.substr (filepath.find_last_of ("/") + 1);
  size_t index_of_dot = filename.find_last_of (".");
  string extension = index_of_dot == string::npos ? "" : filename.substr (index_of_dot + 1);
  string mime_type = _connection->server()->configuration().get<string> ("mime-" + extension, "");
  return mime_type;
}


} // namespace server
} // namespace rosetta
