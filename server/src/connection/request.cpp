
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
#include <vector>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "common/include/date.hpp"
#include "common/include/string_helper.hpp"
#include "server/include/server.hpp"
#include "server/include/connection/request.hpp"
#include "server/include/connection/connection.hpp"
#include "server/include/connection/match_condition.hpp"
#include "server/include/exceptions/request_exception.hpp"
#include "server/include/connection/handlers/request_handler.hpp"

namespace rosetta {
namespace server {
  
using std::string;
using std::vector;
using std::ifstream;
using std::istreambuf_iterator;
using boost::system::error_code;
using boost::algorithm::trim;
using boost::algorithm::split;
using namespace rosetta::common;


request_ptr request::create (connection * connection)
{
  return request_ptr (new request (connection));
}


request::request (connection * connection)
  : _connection (connection)
{ }


void request::handle (exceptional_executor x)
{
  // Figuring out the maximum accepted length of the initial HTTP-Request line.
  auto max_uri_length = _connection->_server->configuration().get<size_t> ("max-uri-length", 4096);
  match_condition match (max_uri_length, "\r\n");

  // Reading initial HTTP-Request line.
  async_read_until (*_connection->_socket, _request_buffer, match, [this, match, x] (const error_code & error, size_t bytes_read) {

    // Checking if socket has an error, or if reading the first line created an error (too long request).
    if (error) {

      // Our socket has an error of some sort, stopping connection early.
      throw request_exception ("Socket error while reading HTTP-Request line.");
    } else if (match.has_error()) {

      // HTTP-Version line was too long, probably too long URI, returning 414 to client.
      write_error_response (414, x);
    } else {

      // Parsing request line.
      string http_request_line = string_helper::get_line (_request_buffer);

      // Splitting initial HTTP line into its three parts.
      vector<string> parts;
      split (parts, http_request_line, ::isspace);

      // Verifying initial line has exactly 3 parts.
      if (parts.size() != 3)
        throw request_exception ("Initial HTTP-Request line was malformed, line contained; '" + http_request_line + "'.");

      // Now we can start deducting which type of request, and path, etc, this is.
      string type         = parts [0];
      string path         = parts [1];
      string version      = parts [2];

      // Decorating request.
      decorate (type, path, version, x, [this] (exceptional_executor x) {
        
        // Reading HTTP headers into request.
        read_headers (x, [this] (exceptional_executor x) {

          // Reading content of HTTP request, if any.
          read_content (x, [this] (exceptional_executor x) {

            // First we need to create our handler.
            _request_handler = request_handler::create (_connection->_server, _connection->_socket, this);

            // Making sure we actually have a request_handler before we start using it.
            // If we do not have a request_handler at this point, then it means that we do not serve this type of request,
            // and hence we return a 404 error.
            if (_request_handler == nullptr) {

              // No handler for this request, returning 404.
              write_error_response (404, x);
            } else {

              // Now we can let our handler take care of the rest of our request, making sure we pass in exceptional_executor,
              // such that if an exception occurs, then the connection is closed.
              _request_handler->handle (x, [] (exceptional_executor x) {

                // Now request is finished handled, and we need to determine if we should keep connection alive, or if
                // we should close the connection immediately.
              });
            }
          });
        });
      });
    }
  });
}


void request::decorate (const string & type,
                        const string & uri,
                        const string & version,
                        exceptional_executor x,
                        function<void(exceptional_executor x)> callback)
{
  // Sanity checking version.
  if (version != "HTTP/1.1") {

    // Unsupported HTTP version.
    write_error_response (501, x);
    return;
  }
  // Storing HTTP version of request.
  _version = version;

  // Checking that this is a type of request we know how to handle.
  if (type != "GET" && type != "POST" && type != "DELETE" && type != "PUT") {

    // We cannot handle this request, hence we return 501 to client.
    write_error_response (501, x);
    return;
  }
  // Storing type of request.
  _type = type;

  // Checking if path is "/" or empty, at which case we default it to the default file, referenced in configuration.
  if (uri.size() == 0 || uri == "/") {

    // Serving default document.
    _uri = _connection->_server->configuration().get<string> ("default-page", "/index.html");
  } else {

    // Checking if URI contains HTTP GET parameters.
    auto index_of_pars = uri.find ("?");
    if (index_of_pars <= 1) {

      // Default page was requested, with HTTP GET parameters.
      parse_parameters (string_helper::decode_uri (uri.substr (index_of_pars + 1)));

      // Serving default document.
      _uri = _connection->_server->configuration().get<string> ("default-page", "/index.html");
    } else if (index_of_pars != string::npos) {

      // URI contains GET parameters.
      parse_parameters (string_helper::decode_uri (uri.substr (index_of_pars + 1)));

      // Decoding actual URI.
      _uri = string_helper::decode_uri (uri.substr (0, index_of_pars));
    } else {

      // No parameters in request URI, making sure we decode the string any way, in case request is for a document with a "funny name".
      _uri = string_helper::decode_uri (uri);
    }
  }

  // Invoking callback supplied for success condition.
  callback (x);
}


void request::parse_parameters (const string & params)
{
  // Splitting up into separate parameters, and looping through each parameter.
  vector<string> pars;
  split (pars, params, boost::is_any_of ("&"));
  for (string & idx : pars) {
    
    // Splitting up name/value of parameter.
    size_t index_of_equal = idx.find ("=");
    string name = index_of_equal == string::npos ? idx : idx.substr (0, index_of_equal);
    string value = index_of_equal == string::npos ? "" : idx.substr (index_of_equal + 1);
    _parameters [name] = value;
  }
}


void request::read_headers (exceptional_executor x, function<void(exceptional_executor x)> functor)
{
  // Sanity check, we don't accept more than "max-header-count" HTTP headers from configuration.
  auto max_header_count = _connection->_server->configuration().get<size_t> ("max-header-count", 25);
  if (_headers.size() > max_header_count) {

    // Writing error status response, and returning early.
    write_error_response (413, x);
    return;
  }

  // Making sure each header don't exceed the maximum length defined in configuration.
  size_t max_header_length = _connection->_server->configuration().get<size_t> ("max-header-length", 8192);
  match_condition match (max_header_length, "\r\n", "\t ");

  // Now we can read the first header from socket, making sure it does not exceed max-header-length
  async_read_until (*_connection->_socket, _request_buffer, match, [this, x, match, functor] (const error_code & error, size_t bytes_read) {

    // Making sure there was no errors while reading socket
    if (error)
      throw request_exception ("Socket error while reading HTTP headers.");

    // Making sure there was no more than maximum number of bytes read according to configuration.
    if (match.has_error ()) {

      // Writing error status response, and returning early.
      write_error_response (413, x);
      return;
    }

    // Now we can start parsing HTTP header.
    string line = string_helper::get_line (_request_buffer, bytes_read);

    // Checking if there are more headers.
    if (line == "") {

      // We're now done reading all HTTP headers, giving control back to connection through function callback.
      functor (x);
    } else {

      // There are possibly more HTTP headers, continue reading until we see an empty line.
      string key, value;
      const size_t equals_idx = line.find (':');
      if (string::npos == equals_idx) {

        // Missing colon (:) in HTTP header.
        throw request_exception ("Syntes error in HTTP header close to; '" + line + "'");
      } else {

        key = line.substr (0, equals_idx);
        value = line.substr (equals_idx + 1);
      }

      // Trimming header key and value before we put results into collection.
      trim (key);
      trim (value);
      _headers [key] = value;

      // Fetching next header by invoking self.
      read_headers (x, functor);
    }
  });
}


void request::read_content (exceptional_executor x, function<void(exceptional_executor x)> functor)
{
  // Checking if there is any content first.
  string content_length_str = (*this)["Content-Length"];

  // Checking if there is any Content-Length
  if (content_length_str == "") {

    // No content, invoking callback functor immediately.
    functor (x);
  } else {

    // Checking that content does not exceed max request content length, defaulting to 16 MB.
    auto content_length = lexical_cast<size_t> (content_length_str);
    auto max_content_length = _connection->_server->configuration().get<size_t> ("max-request-content-length", 4194304);
    if (content_length > max_content_length) {

      // Writing error status response, and returning early.
      write_error_response (500, x);
      return;
    }

    // Reading content into streambuf.
    match_condition match (content_length);
    async_read_until (*_connection->_socket, _request_buffer, match, [this, match, content_length, functor, x] (const error_code & error, size_t bytes_read) {

      // Checking for socket errors.
      if (error)
        throw request_exception ("Socket error while reading content of request.");

      // Verifying that number of bytes read was the same as the number of bytes specified in Content-Length header.
      if (bytes_read != content_length)
        throw request_exception ("Not enough bytes in request content to match the Content-Length header.");

      // Invoking functor callback supplied by caller.
      // Notice, at this point, we simply keep the content in our connection's streambuf for later references.
      functor (x);
    });
  }
}


const string & request::operator [] (const string & key) const
{
  // String we return if there is no HTTP header with the specified name.
  const static string empty_return_value = "";

  // Checking if we have the specified HTTP header.
  auto index = _headers.find (key);
  if (index == _headers.end ())
    return empty_return_value; // No such header.

  return index->second;
}


void request::write_error_response (int status_code, exceptional_executor x)
{
  // Creating status line, and serializing to socket.
  string status_line = "HTTP/1.1 " + lexical_cast<string> (status_code) + " ";
  switch (status_code) {
  case 403:
    status_line += "Forbidden";
    break;
  case 404:
    status_line += "Not Found";
    break;
  case 413:
    status_line += "Request Header Too Long";
    break;
  case 414:
    status_line += "Request-URI Too Long";
    break;
  case 500:
    status_line += "Internal Server Error";
    break;
  case 501:
    status_line += "Not Implemented";
    break;
  default:
    status_line += "Unknown Error Type";
    break;
  }
  status_line += "\r\n";

  // Writing status line to socket.
  async_write (*_connection->_socket, buffer (status_line), [this, x, status_code] (const error_code & error, size_t bytes_written) {

    // Sanity check.
    if (error)
      throw request_exception ("Socket error while returning HTTP error status line back to client.");

    // Figuring out error file to use, and its size.
    string path = "error-pages/" + lexical_cast<string> (status_code) + ".html";
    size_t size = boost::filesystem::file_size (path);

    // Writing default headers for an error request.
    string headers = "Date: " + date::now ().to_string () + "\r\n";
    headers       += "Content-Type: text/html; charset=utf-8\r\n";
    headers       += "Content-Length: " + lexical_cast<string> (size) + "\r\n\r\n";
    async_write (*_connection->_socket, buffer (headers), [this, x, path] (const error_code & error, size_t bytes_written) {

      // Sanity check.
      if (error)
        throw request_exception ("Socket error while returning HTTP headers back to client on error request.");

      // Writing file to socket.
      ifstream fs (path, std::ios_base::binary);
      vector<char> file_content ((istreambuf_iterator<char> (fs)), istreambuf_iterator<char>());
      async_write (*_connection->_socket, buffer (file_content), [x] (const error_code & error, size_t bytes_written) {

        // Sanity check.
        if (error)
          throw request_exception ("Socket error while returning error file content back to client.");

        // Intentionally not releasing exceptional_executor, to make sure we close socket, since this was an error request.
      });
    });
  });
}


} // namespace server
} // namespace rosetta
