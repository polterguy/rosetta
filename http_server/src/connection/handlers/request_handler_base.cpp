
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


void request_handler_base::write_file_headers (path filepath, bool last_modified, exceptional_executor x, functor on_success)
{
  // Figuring out size of file, and making sure it's not larger than what we are allowed to handle according to configuration of server.
  size_t size = file_size (filepath);

  // Retrieving MIME type, and verifying this is a type of file we actually serve.
  string mime_type = get_mime (filepath);
  if (mime_type == "") {

    // File type is not served according to configuration of server.
    _request->write_error_response (x, 403);
    return;
  }

  // Building our standard response headers for a file transfer.
  collection headers {
    {"Content-Type", mime_type},
    {"Content-Length", boost::lexical_cast<string> (size)}};

  // Checking if caller wants to add "Las-Modified" header to envelope.
  if (last_modified)
    headers.push_back ({"Last-Modified", date::from_file_change (filepath.string ()).to_string ()});

  // Writing special handler headers to connection.
  write_headers (headers, x, [this, on_success] (auto x) {

    // Invoking on_success() supplied by caller.
    on_success (x);
  });
}


void request_handler_base::write_file (path filepath, unsigned int status_code, bool last_modified, exceptional_executor x, functor on_success)
{
  // Making things slightly more tidy in here.
  using namespace std;

  // Retrieving MIME type, and verifying this is a type of file we actually serve.
  string mime_type = get_mime (filepath);
  if (mime_type == "") {

    // File type is not served according to configuration of server.
    _request->write_error_response (x, 403);
    return;
  }

  // Writing status code.
  write_status (status_code, x, [this, filepath, on_success, last_modified] (auto x) {

    // Writing special file headers back to client.
    write_file_headers (filepath, last_modified, x, [this, filepath, on_success] (auto x) {

      // Writing standard headers to client.
      write_standard_headers (x, [this, filepath, on_success] (auto x) {

        // Make sure we close envelope.
        ensure_envelope_finished (x, [this, filepath, on_success] (auto x) {

          // Opening up file, as a shared_ptr, passing it into write_file(),
          // such that file stays around, until all bytes have been written.
          shared_ptr<std::ifstream> fs_ptr = make_shared<std::ifstream> (filepath.string (), ios::in | ios::binary);
          if (!fs_ptr->good())
            throw request_exception ("Couldn't open file; '" + filepath.string () + "' for reading.");

          // Writing actual file.
          write_file (fs_ptr, x, on_success);
        });
      });
    });
  });
}


string request_handler_base::get_mime (path filename)
{
  // Then we do a lookup into the configuration for our server, to see if it has defined a MIME type for the given file's extension.
  string mime_type = _connection->server()->configuration().get<string> ("mime" + filename.extension().string (), "");

  // Returning MIME type to caller.
  return mime_type;
}


void request_handler_base::write_file (shared_ptr<std::ifstream> fs_ptr, exceptional_executor x, functor on_success)
{
  // Checking if we're done.
  if (fs_ptr->eof()) {

    // Yup, we're done!
    on_success (x);
  } else {

    // Reading from file into array.
    fs_ptr->read (_response_buffer.data(), _response_buffer.size());

    // Creating a buffer from the _response_buffer std::array.
    auto bf = buffer (_response_buffer.data(), fs_ptr->gcount());

    // Writing buffer to socket, making sure we pass in shared_ptr to file stream, such that it stays around until we're entirely finished.
    // Notice, this method will not read entire file into memory, but rather read 8192 bytes from the file, and flush these bytes to the
    // socket, for then to invoke "self" multiple times, until entire file has been served over socket, back to client.
    // This conserves memory and resources on the server, but also makes sure the file is open for a longer period.
    // However, to make it possible to retrieve very large files, without completely exhausting the server's resources, this is our choice.
    _connection->socket().async_write (bf, [this, on_success, x, fs_ptr] (auto error, auto bytes_written) {

      // Sanity check.
      if (error)
        throw request_exception ("Socket error while writing file.");

      // Invoking self.
      write_file (fs_ptr, x, on_success);
    });
  }
}


} // namespace http_server
} // namespace rosetta
