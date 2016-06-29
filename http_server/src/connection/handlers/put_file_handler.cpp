
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

#include <boost/filesystem.hpp>
#include "http_server/include/server.hpp"
#include "http_server/include/helpers/date.hpp"
#include "http_server/include/connection/request.hpp"
#include "http_server/include/connection/connection.hpp"
#include "http_server/include/exceptions/request_exception.hpp"
#include "http_server/include/connection/handlers/put_file_handler.hpp"

namespace rosetta {
namespace http_server {

using std::string;
using namespace rosetta::common;


put_file_handler::put_file_handler (class connection * connection, class request * request)
  : request_handler_base (connection, request)
{ }


void put_file_handler::handle (exceptional_executor x, functor on_success)
{
  // Retrieving URI from request, removing initial "/", before prepending it with the www-root folder.
  auto uri = request()->envelope().path();

  // Retrieving root path, and building full path for document.
  const string WWW_ROOT_PATH = connection()->server()->configuration().get<string> ("www-root", "www-root/");
  const string filename = WWW_ROOT_PATH + uri.c_str();

  // Writing file to server.
  save_request_content (filename, x, on_success);
}


void put_file_handler::save_request_content (const string & filename, exceptional_executor x, functor on_success)
{
  // Making things more tidy in here.
  using namespace std;

  // Setting deadline timer for content read.
  const int CONTENT_READ_TIMEOUT = connection()->server()->configuration().get<size_t> ("request-content-read-timeout", 300);
  connection()->set_deadline_timer (CONTENT_READ_TIMEOUT);

  // Retrieving Content-Length of request.
  size_t content_length = get_content_length ();

  // Creating file, to pass in as shared_ptr, to make sure it stays valid, until process is finished.
  auto file_ptr = make_shared<std::ofstream> (filename + ".partial", ios::binary | ios::trunc | ios::out);

  // Creating an input stream wrapping the asio stream buffer.
  auto ss_ptr = make_shared<istream> (&connection()->buffer());

  // Creating exceptional_executor, to make sure file becomes deleted, unless entire operation succeeds.
  exceptional_executor x2 ([file_ptr, filename] () {

    // Closing existing file pointer, and deleting file, since operation was not successful.
    file_ptr->close ();
    boost::system::error_code ec;
    boost::filesystem::remove (filename, ec);
  });

  // Invoking implementation, that reads from socket, and saves to file.
  save_request_content_to_file (file_ptr, ss_ptr, content_length, x, x2, [this, filename, on_success] (auto x) {

    // Renaming file from its temporary name.
    boost::filesystem::rename (filename + ".partial", filename);

    // Returning success to client.
    write_success_envelope (x, on_success);
  });
}


void put_file_handler::save_request_content_to_file (std::shared_ptr<std::ofstream> file_ptr,
                                                     std::shared_ptr<std::istream> ss_ptr,
                                                     size_t content_length,
                                                     exceptional_executor x,
                                                     exceptional_executor x2,
                                                     functor on_success)
{
  // Making sure we read content in chunks of BUFFER_SIZE (8192 bytes) from stream buffer.
  size_t chunk_size = content_length > BUFFER_SIZE ? BUFFER_SIZE : content_length;
  content_length = content_length > BUFFER_SIZE ? content_length - BUFFER_SIZE : 0;

  // Reading next chunk from socket.
  connection()->socket().async_read (connection()->buffer(),
                                     transfer_exactly (chunk_size),
                                     [this, file_ptr, ss_ptr, content_length, x, x2, on_success] (auto error, auto bytes_read) {

    // Checking for socket errors.
    if (error == error::operation_aborted)
      return;
    if (error)
      throw request_exception ("Socket error while reading request content.");

    // Then reading from wrapped input stream, and flushing to output file.
    ss_ptr->readsome (_file_buffer.data(), _file_buffer.size());
    file_ptr->write (_file_buffer.data(), ss_ptr->gcount ());

    // Checking if we have more bytes to read, and if so, invoke self.
    if (content_length > 0) {

      // Reading next chunk.
      save_request_content_to_file (file_ptr, ss_ptr, content_length, x, x2, on_success);

    } else {

      // Releasing "delete file exceptional_executor".
      x2.release();

      // Closing output file.
      file_ptr->close ();

      // Invoking functor callback supplied by caller.
      on_success (x);
    }
  });
}


size_t put_file_handler::get_content_length ()
{
  // Max allowed length of content.
  const size_t MAX_REQUEST_CONTENT_LENGTH = connection()->server()->configuration().get<size_t> ("max-request-content-length", 4194304);

  // Checking if there is any content first.
  string content_length_str = request()->envelope().header ("Content-Length");

  // Checking if there is any Content-Length
  if (content_length_str.size() == 0) {

    // No content.
    throw request_exception ("No Content-Length header in PUT request.");
  } else {

    // Checking that Content-Length does not exceed max request content length.
    auto content_length = boost::lexical_cast<size_t> (content_length_str);
    if (content_length > MAX_REQUEST_CONTENT_LENGTH)
      throw request_exception ("Too much content in request for server to handle.");

    // Checking that Content-Length is not 0, or negative.
    if (content_length <= 0)
      throw request_exception ("No content provided to PUT handler.");

    // Returning Content-Length to caller.
    return content_length;
  }
}


void put_file_handler::write_success_envelope (exceptional_executor x, functor on_success)
{
  // Writing status code success back to client.
  write_status (200, x, [this, on_success] (auto x) {

    // Writing standard headers back to client.
    write_standard_headers (x, [this, on_success] (auto x) {

      // Ensuring envelope is closed.
      ensure_envelope_finished (x, on_success);
    });
  });
}


} // namespace http_server
} // namespace rosetta
