
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
#include <algorithm>
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
using boost::system::error_code;
using namespace rosetta::common;


request_ptr request::create (connection * connection, boost::asio::streambuf & buffer)
{
  return request_ptr (new request (connection, buffer));
}


request::request (connection * connection, boost::asio::streambuf & buffer)
  : _connection (connection),
    _request_buffer (buffer)
{ }


void request::handle (exceptional_executor x)
{
  // Figuring out the maximum accepted length of the initial HTTP-Request line, before reading initial HTTP-Request line.
  auto max_uri_length = _connection->_server->configuration().get<size_t> ("max-uri-length", 4096);
  match_condition match (max_uri_length);
  async_read_until (*_connection->_socket, _request_buffer, match, [this, match, x] (const error_code & error, size_t bytes_read) {

    // Checking if socket has an error, or if reading the first line, created an error (Too Long Request)
    if (error) {

      // Our socket was probably closed due to a timeout.
      x.release ();
    } else if (match.has_error()) {

      // HTTP-Version line was too long, URI was too long, returning 414 to client.
      write_error_response (414, x);
    } else {

      // Parsing request line.
      string http_request_line = string_helper::get_line (_request_buffer);

      // Splitting initial HTTP line into its three parts.
      vector<string> parts;
      boost::algorithm::split (parts, http_request_line, ::isspace);

      // Removing all empty parts of URI, meaning consecutive spaces.
      std::remove (parts.begin(), parts.end(), "");

      // Checking if the HTTP-Request line contains more than 3 entities, and if so, default behavior is to assume the URL has spaces.
      while (parts.size () > 3) {

        // Putting back any plain-text SP, assuming their SP in the URI of the request.
        parts [1] += " " + parts [2];
        parts.erase (parts.begin () + 2);
      }

      // Now we can start deducting which type of request, and path, etc, this is, trying to be as fault tolerant as we can, to
      // support a maximum amount of HTTP "cloaking".
      size_t no_parts = parts.size ();
      string type         = no_parts > 1 ? boost::algorithm::to_upper_copy (parts [0]) : "GET";
      string uri          = no_parts > 1 ? parts [1] : parts [0];
      string version      = no_parts > 2 ? boost::algorithm::to_upper_copy (parts [2]) : "HTTP/1.1";

      // Decorating request.
      decorate (type, uri, version, x, [this] (exceptional_executor x) {

        // Reading HTTP headers into request.
        _connection->set_deadline_timer (_connection->_server->configuration().get<size_t> ("request-header-read-timeout", 10));
        read_headers (x, [this] (exceptional_executor x) {

          // Reading content of HTTP request, if any.
          read_content (x, [this] (exceptional_executor x) {

            // Killing timer, since we're finished reading from client.
            _connection->kill_deadline_timer ();

            // First we need to create our handler.
            _request_handler = request_handler::create (_connection->_server, _connection->_socket, this);
            if (_request_handler == nullptr) {

              // No handler for this request, returning 404.
              write_error_response (404, x);
            } else {

              // Letting our request_handler take care of the rest.
              _request_handler->handle (x, [this] (exceptional_executor x) {

                // Now request is finished handled, and we can take back control over connection.
                string connection = boost::algorithm::to_lower_copy ((*this)["connection"]);
                if (connection != "close") {

                  // Keeping connection alive, by invoking release() on "x", and letting our connection class take over from here.
                  x.release ();
                  _connection->keep_alive();
                }

                // Else; "x" will simply go out of scope, releasing the connection, closing the socket, etc ...
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
  // Storing HTTP version of request.
  if (version.find_first_of ("23456789") != string::npos)
    _version = "HTTP/2.0"; // Creating an HTTP/2.0 request, if version string contains a number higher than "1".
  else
    _version = "HTTP/1.1"; // No version information provided, default behavior is to assume HTTP/1.1

  // Storing type of request.
  _type = type;

  // Checking if path is one character or less, and if so, defaulting to "default-page" from configuration.
  if (uri.size() <= 1) {

    // Serving default document.
    _uri = _connection->_server->configuration().get<string> ("default-page", "/index.html");
  } else {

    // Checking if URI contains HTTP GET parameters, allowing for multiple different GET parameter delimiters, to support
    // maximum amount of HTTP cloaking.
    auto index_of_pars = uri.find_first_of ("?*$~^€§");
    if (index_of_pars <= 1) {

      // Default page was requested, with HTTP GET parameters.
      parse_parameters (string_helper::decode_uri (uri.substr (index_of_pars + 1)));

      // Serving default document.
      _uri = _connection->_server->configuration().get<string> ("default-page", "/index.html");
    } else if (index_of_pars != string::npos) {

      // URI contains GET parameters.
      parse_parameters (string_helper::decode_uri (uri.substr (index_of_pars + 1)));

      // Decoding actual URI, and making sure it starts with a "/".
      if (uri.find_first_of ("/") != 0)
        _uri = "/" + string_helper::decode_uri (uri.substr (0, index_of_pars));
      else
        _uri = string_helper::decode_uri (uri.substr (0, index_of_pars));
    } else {

      // Decoding actual URI, and making sure it starts with a "/".
      if (uri.find_first_of ("/") != 0)
        _uri = "/" + string_helper::decode_uri (uri);
      else
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
  // Valid HTTP header name characters.
  const static string HTTP_HEADER_VALID_CHARACTERS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-";

  // Making sure each header don't exceed the maximum length defined in configuration.
  size_t max_header_length = _connection->_server->configuration().get<size_t> ("max-header-length", 8192);
  match_condition match (max_header_length);

  // Now we can read the first header from socket, making sure it does not exceed max-header-length
  async_read_until (*_connection->_socket, _request_buffer, match, [this, x, match, functor] (const error_code & error, size_t bytes_read) {

    // Making sure there was no errors while reading socket
    if (error) {

      // Our connection was probably dropped due to a timeout.
      x.release ();
      return;
    }

    // Making sure there was no more than maximum number of bytes read according to configuration.
    if (match.has_error ()) {

      // Writing error status response, and returning early.
      write_error_response (413, x);
      return;
    }

    // Now we can start parsing HTTP headers.
    string line = string_helper::get_line (_request_buffer, bytes_read);
    if (line.size () == 0) {

      // No more headers.
      functor (x);
      return;
    }

    // There are possibly more HTTP headers, continue reading until we see an empty line.
    const auto equals_idx = std::find_if (line.begin (), line.end (), [] (char idx) {
      return (idx >= 'a' && idx <= 'z') || (idx >= 'A' && idx <= 'Z') || (idx >= '0' && idx <= '9') || idx == '-';
    });
    if (equals_idx == line.end ()) {

      // Missing colon (:) in HTTP header, meaning, only HTTP-Header name, and no value.
      // To be more fault tolerant towards non-conforming clients, we still let this one pass.
      _headers [boost::algorithm::to_lower_copy (boost::algorithm::trim_copy (line))] = "";
    } else {

      // Both name and key was supplied.
      _headers [boost::algorithm::to_lower_copy (boost::algorithm::trim_copy (string (line.begin (), equals_idx)))]
        = boost::algorithm::trim_copy (string (equals_idx + 1, line.end ()));
    }

    // Reading next header from socket.
    read_headers (x, functor);
  });
}


void request::read_content (exceptional_executor x, function<void(exceptional_executor x)> functor)
{
  // Checking if there is any content first.
  string content_length_str = (*this)["content-length"];

  // Checking if there is any Content-Length
  if (content_length_str == "") {

    // No content, killing timer before invoking callback functor.
    _connection->kill_deadline_timer ();
    functor (x);
  } else {

    // Setting deadline timer to content-read value.
    _connection->set_deadline_timer (_connection->_server->configuration().get<size_t> ("request-content-read-timeout", 300));

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
    /*async_read_until (*_connection->_socket, _request_buffer, match, [this, match, content_length, functor, x] (const error_code & error, size_t bytes_read) {

      // Checking for socket errors.
      if (error)
        throw request_exception ("Socket error while reading content of request.");

      // Verifying that number of bytes read was the same as the number of bytes specified in Content-Length header.
      if (bytes_read != content_length)
        throw request_exception ("Not enough bytes in request content to match the Content-Length header.");

      // Invoking functor callback supplied by caller.
      // Notice, at this point, we simply keep the content in our connection's streambuf for later references.
      functor (x);
    });*/
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

  // Making sure we kill deadline timer, before we start another async task.
  _connection->kill_deadline_timer ();

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
      std::ifstream fs (path, std::ios_base::binary);
      vector<char> file_content ((std::istreambuf_iterator<char> (fs)), std::istreambuf_iterator<char>());
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
