
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

#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "common/include/string_helper.hpp"
#include "common/include/date.hpp"
#include "server/include/connection/request.hpp"
#include "server/include/connection/connection.hpp"
#include "server/include/connection/match_condition.hpp"
#include "server/include/exceptions/request_exception.hpp"

namespace rosetta {
namespace server {
  
using std::move;
using std::vector;
using std::ifstream;
using std::shared_ptr;
using std::istreambuf_iterator;
using boost::split;
using boost::system::error_code;
using boost::posix_time::seconds;
using boost::algorithm::to_lower_copy;
using namespace rosetta::common;


connection_ptr connection::create (class server * server, socket_ptr socket)
{
  return connection_ptr (new connection (server, socket));
}


connection::connection (class server * server, socket_ptr socket)
  : _server (server),
    _socket (socket)
{ }


connection::~connection ()
{ }


void connection::handle()
{
  // Figuring out the maximum accepted length of the initial HTTP-Request line.
  auto max_uri_length = _server->configuration().get<size_t> ("max-uri-length", 4096);
  match_condition match (max_uri_length, "\r\n");

  // Reading initial HTTP-Request line.
  async_read_until (*_socket, _request_buffer, match, [this, match] (const error_code & error, size_t bytes_read) {

    // Making sure connection is closed if an exception occurs before we leave function.
    auto x = ensure_close ();

    // Checking if socket has an error, or if reading the first line created an error (too long request).
    if (error) {

      // Our socket has an error of some sort, stopping connection early.
      throw request_exception ("Socket error while reading HTTP-Request line.");
    } else if (match.has_error()) {

      // HTTP-Version line was too long, returning 414 to client.
      write_error_response (414, x);
    } else {

      // Parsing request line.
      string http_request_line = string_helper::get_line (_request_buffer);

      // Splitting initial HTTP line into its three parts.
      vector<string> parts;
      split (parts, http_request_line, ::isspace);

      // Verifying initial line has exactly 3 parts.
      if (parts.size() != 3) {

        // Some sort of error in initial HTTP line from client.
        throw request_exception ("Initial HTTP-Request line was malformed, line contained; '" + http_request_line + "'.");
      }

      // Now we can start deducting which type of request, and path, etc, this is.
      string type         = to_lower_copy (parts [0]);
      string path         = parts [1];
      string version      = to_lower_copy (parts [2]);

      // Creating our request.
      _request = request::create (this, type, path, version);

      // Reading HTTP headers into request.
      _request->read_headers (x, [this] (exceptional_executor x) {

        // Reading content of HTTP request, if any.
        _request->read_content (x, [this] (exceptional_executor x) {

          // We are now finished reading our request, and we can create a request handler, responsible of creating a response.
          _request->handle (x, [this] (exceptional_executor x) {

            // Request is now finished handled, and some sort of response has been returned to client.
            // Now we hand over control to method responsible for either closing connection, or keeping connection alive for next request.
          });
        });
      });
    }
  });
}


void connection::stop ()
{
  error_code ignored_ec;
  _socket->shutdown (tcp::socket::shutdown_both, ignored_ec);
  _socket->close ();

  // Making sure we delete connection from its manager.
  _server->remove_connection (shared_from_this());
}


exceptional_executor connection::ensure_close ()
{
  return exceptional_executor ([this] () {
   stop ();
  });
}


void connection::write_error_response (int status_code, exceptional_executor x)
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
  async_write (*_socket, buffer (status_line), [this, x, status_code] (const error_code & error, size_t bytes_written) {

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
    async_write (*_socket, buffer (headers), [this, x, path] (const error_code & error, size_t bytes_written) {

      // Sanity check.
      if (error)
        throw request_exception ("Socket error while returning HTTP headers back to client on error request.");

      // Writing file to socket.
      ifstream fs (path, std::ios_base::binary);
      vector<char> file_content ((istreambuf_iterator<char> (fs)), istreambuf_iterator<char>());
      async_write (*_socket, buffer (file_content), [x] (const error_code & error, size_t bytes_written) {

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
