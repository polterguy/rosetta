
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
#include "server/include/connection/handlers/head_handler.hpp"
#include "server/include/connection/handlers/error_handler.hpp"
#include "server/include/connection/handlers/trace_handler.hpp"
#include "server/include/connection/handlers/request_handler.hpp"
#include "server/include/connection/handlers/redirect_handler.hpp"
#include "server/include/connection/handlers/static_file_handler.hpp"

namespace rosetta {
namespace server {

using std::string;
using boost::system::error_code;
using namespace boost::asio;
using namespace rosetta::common;


request_handler_ptr request_handler::create (class connection * connection, class request * request, int status_code)
{
  // Retrieving whether or not we should upgrade insecure requests automatically.
  const static bool upgrade_insecure_requests = connection->server()->configuration().get <bool> ("upgrade-insecure-requests", false);
  const static string ssl_port = connection->server()->configuration().get <string> ("ssl-port", "443");
  const static string server_address = connection->server()->configuration().get <string> ("address", "localhost");

  // Checking request type, and other parameters, deciding which type of request handler we should create.
  if (status_code >= 400) {

    // Some sort of error.
    return request_handler_ptr (new error_handler (connection, request, status_code));

  } else if (upgrade_insecure_requests && !connection->is_secure() && request->envelope().header ("Upgrade-Insecure-Requests") == "1") {

    // Both configuration, and client, prefers secure requests, and current connection is not secure.
    // Redirecting client to SSL version of same resource.
    string request_uri = request->envelope().uri();
    if (request_uri == connection->server()->configuration().get <string> ("default-page", "/index.html"))
      request_uri = "/";
    string uri = "https://" + server_address + (ssl_port == "443" ? "" : ":" + ssl_port) + request_uri;
    return request_handler_ptr (new redirect_handler (connection, request, 307, uri, true));

  } else if (request->envelope().type() == "TRACE") {

    // Checking if TRACE method is allowed according to configuration.
    if (!connection->server()->configuration().get<bool> ("trace-allowed", false)) {

      // Method not allowed.
      return request_handler_ptr (new error_handler (connection, request, 405));
    } else {

      // Creating a TRACE response handler.
      return request_handler_ptr (new trace_handler (connection, request));
    }
  } else if (request->envelope().type() == "HEAD") {

    // Checking if HEAD method is allowed according to configuration.
    if (!connection->server()->configuration().get<bool> ("head-allowed", false)) {

      // Method not allowed.
      return request_handler_ptr (new error_handler (connection, request, 405));
    } else {

      // Creating a HEAD response handler.
      return request_handler_ptr (new head_handler (connection, request, request->envelope().extension()));
    }
  } else if (request->envelope().type() == "GET") {

    // Figuring out handler to use according to request extension, and if document type even is served/handled.
    const string & extension = request->envelope().extension();
    string handler = extension.size () == 0 ?
        connection->server()->configuration().get<string> ("default-handler", "error") :
        connection->server()->configuration().get<string> (extension + "-handler", "error");

    // Returning the correct handler to caller.
    if (handler == "static-file-handler") {

      // Static file handler.
      return request_handler_ptr (new static_file_handler (connection, request, extension));
    } else {

      // Oops, these types of files are not served or handled.
      return request_handler_ptr (new error_handler (connection, request, 404));
    }
  } else {

    // Unsupported method.
    return request_handler_ptr (new error_handler (connection, request, 405));
  }
}


request_handler::request_handler (class connection * connection, class request * request)
  : _connection (connection),
    _request (request)
{ }


void request_handler::write_status (unsigned int status_code, exceptional_executor x, functor callback)
{
  // Creating status line, and serializing to socket, making sure status_line stays around until after write operation is finished.
  std::shared_ptr<string> status_line = std::make_shared<string> ("HTTP/1.1 " + boost::lexical_cast<string> (status_code) + " ");
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
    if (status_code > 200 && status_code < 300)
      *status_line += "Unknown Success Type";
    else if (status_code >= 300 && status_code < 400)
      *status_line += "Unknown Type";
    else
      *status_line += "Unknown Error Type";
    break;
  }
  *status_line += "\r\n";

  // Writing status line to socket.
  _connection->socket().async_write (buffer (*status_line), [callback, x, status_line] (const error_code & error, size_t bytes_written) {

    // Sanity check.
    if (error)
      throw request_exception ("Socket error while writing HTTP status line.");
    else
      callback (x);
  });
}


void request_handler::write_header (const string & key, const string & value, exceptional_executor x, functor callback, bool is_last)
{
  // Creating header, making sure the string stays around until after socket write operation is finished.
  std::shared_ptr<string> header_content = std::make_shared<string> (key + ": " + value + "\r\n");

  // Checking if this was our last header, and if so, appending an additional CR/LF sequence.
  if (is_last) {

    // Making sure we submit the server name back to client.
    *header_content += "Server: Rosetta\r\n"; // Notice, we do not supply a version number to make it more difficult for malware to exploit server!
    *header_content += "\r\n";
  }

  // Writing header content to socket.
  _connection->socket().async_write (buffer (*header_content), [callback, x, header_content] (const error_code & error, size_t bytes_written) {

    // Sanity check.
    if (error)
      throw request_exception ("Socket error while writing HTTP header.");
    else
      callback (x);
  });
}


void request_handler::write_headers (header_list headers, exceptional_executor x, functor callback, bool is_last)
{
  if (headers.size() == 0) {

    // No more headers.
    if (callback != nullptr)
      callback (x);
  } else {

    // Retrieving next key/value pair.
    string key = std::get<0> (headers [0]);
    string value = std::get<1> (headers [0]);

    // Popping off currently written header from list of headers.
    headers.erase (headers.begin (), headers.begin () + 1);

    // Writing header.
    write_header (key, value, x, [this, headers, callback, is_last] (exceptional_executor x) {

      // Invoking self.
      write_headers (headers, x, callback, is_last);
    }, headers.size() == 0 ? is_last : false);
  }
}


void request_handler::write_file (const string & filepath, exceptional_executor x, functor callback, bool write_content)
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

  // Opening up file, as a shared_ptr, making sure it stays around until all bytes have been written, and invoking actual implementation of file writing.
  std::shared_ptr<std::ifstream> fs_ptr = std::make_shared<std::ifstream> (filepath, std::ios::in | std::ios::binary);
  if (!fs_ptr->good())
    throw request_exception ("Couldn't open file; '" + filepath + "' for reading.");

  // Building our request headers.
  header_list headers {
    {"Content-Type", mime_type },
    {"Date", date::now ().to_string ()},
    {"Content-Length", boost::lexical_cast<string> (size)}};

  // Writing HTTP headers to connection.
  write_headers (headers, x, [this, fs_ptr, callback, write_content] (exceptional_executor x) {

    // Invoking implementation that actually writes file to socket.
    if (write_content)
      write_file (fs_ptr, x, callback);
    else
      callback (x); // Not writing content of file.
  }, true);
}


void request_handler::write_file (std::shared_ptr<std::ifstream> fs_ptr, exceptional_executor x, functor callback)
{
  // Checking if we're done.
  if (fs_ptr->eof()) {

    // Yup, we're done!
    callback (x);
  } else {

    // Reading from file into array.
    fs_ptr->read (_response_buffer.data(), _response_buffer.size());

    // Writing to socket, passing in fs_ptr, to make sure it stays around until operation is entirely done.
    auto bf = buffer (_response_buffer.data(), fs_ptr->gcount());
    _connection->socket().async_write (bf, [this, callback, x, fs_ptr] (const error_code & error, size_t bytes_written) {

      // Sanity check.
      if (error)
        throw request_exception ("Socket error while writing file.");

      // Invoking self.
      write_file (fs_ptr, x, callback);
    });
  }
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
