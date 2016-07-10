
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
#include "common/include/exceptional_executor.hpp"
#include "http_server/include/server.hpp"
#include "http_server/include/connection/request.hpp"
#include "http_server/include/connection/connection.hpp"
#include "http_server/include/connection/handlers/put_file_handler.hpp"

namespace rosetta {
namespace http_server {

using std::string;
using namespace rosetta::common;


put_file_handler::put_file_handler (class request * request)
  : content_request_handler (request)
{ }


void put_file_handler::handle (connection_ptr connection, std::function<void()> on_success)
{
  // Retrieving URI from request.
  auto path = request()->envelope().path();
  save_request_content (connection, path, on_success);
}


void put_file_handler::save_request_content (connection_ptr connection, path filename, std::function<void()> on_success)
{
  // Making things more tidy in here.
  using namespace std;

  // Setting deadline timer for content read.
  const int CONTENT_READ_TIMEOUT = connection->server()->configuration().get<int> ("request-content-read-timeout", 300);
  connection->set_deadline_timer (CONTENT_READ_TIMEOUT);

  // Retrieving Content-Length of request.
  size_t content_length = get_content_length (connection);
  if (content_length == 0) {

    // This is a logical error.
    request()->write_error_response (connection, 500);
  } else {

    // Creating file, to pass in as shared_ptr, to make sure it stays valid, until process is finished.
    auto file_ptr = make_shared<std::ofstream> (filename.string () + ".partial", ios::binary | ios::trunc | ios::out);

    // Creating an input stream wrapping the asio stream buffer.
    auto ss_ptr = make_shared<istream> (&connection->buffer());

    // Creating exceptional_executor, to make sure file becomes deleted, unless entire operation succeeds.
    exceptional_executor x ([file_ptr, filename] () {

      // Closing existing file pointer, and deleting file, since operation was not successful.
      file_ptr->close ();
      boost::system::error_code ec;
      boost::filesystem::remove (filename, ec);
    });

    // Invoking implementation, that reads from socket, and saves to file.
    save_request_content_to_file (connection, file_ptr, ss_ptr, content_length, x, [this, connection, filename, on_success] () {

      // Renaming file from its temporary name.
      boost::filesystem::rename (filename.string () + ".partial", filename);

      // Returning success to client.
      write_success_envelope (connection, on_success);
    });
  }
}


void put_file_handler::save_request_content_to_file (connection_ptr connection,
                                                     std::shared_ptr<std::ofstream> file_ptr,
                                                     std::shared_ptr<std::istream> ss_ptr,
                                                     size_t content_length,
                                                     exceptional_executor x,
                                                     std::function<void()> on_success)
{
  // Making sure we read content in chunks of BUFFER_SIZE (8192 bytes) from stream buffer.
  size_t chunk_size = content_length > BUFFER_SIZE ? BUFFER_SIZE : content_length;
  content_length = content_length > BUFFER_SIZE ? content_length - BUFFER_SIZE : 0;

  // Reading next chunk from socket.
  connection->socket().async_read (connection->buffer(),
                                     transfer_exactly (chunk_size),
                                     [this, connection, file_ptr, ss_ptr, content_length, x, on_success] (auto error, auto bytes_read) {

    // Checking for socket errors.
    if (error) {

      // Something went wrong.
      connection->close();
    } else {

      // Making sure we close connection, in case an exception occurs.
      exceptional_executor x2 ([connection] () { connection->close(); });

      // Then reading from wrapped input stream, and flushing to output file.
      ss_ptr->readsome (_file_buffer.data(), _file_buffer.size());
      file_ptr->write (_file_buffer.data(), ss_ptr->gcount ());

      // Checking if we have more bytes to read, and if so, invoke self.
      if (content_length > 0) {

        // Reading next chunk.
        save_request_content_to_file (connection, file_ptr, ss_ptr, content_length, x, on_success);

      } else {

        // Releasing "delete file exceptional_executor".
        x.release();

        // Closing output file.
        file_ptr->close ();

        // Invoking functor callback supplied by caller.
        on_success ();
      }

      // Releasing exception helper.
      x2.release();
    }
  });
}


} // namespace http_server
} // namespace rosetta
