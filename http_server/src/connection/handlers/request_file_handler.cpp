
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
#include "http_server/include/connection/handlers/request_file_handler.hpp"

using std::string;
using std::shared_ptr;
using std::make_shared;
using namespace boost::asio;
using namespace rosetta::common;

namespace rosetta {
namespace http_server {


request_file_handler::request_file_handler (class connection * connection, class request * request)
  : request_handler_base (connection, request)
{ }


void request_file_handler::write_file_headers (path filepath, bool last_modified, exceptional_executor x, functor on_success)
{
  // Figuring out size of file, and making sure it's not larger than what we are allowed to handle according to configuration of server.
  size_t size = file_size (filepath);

  // Retrieving MIME type, and verifying this is a type of file we actually serve.
  string mime_type = get_mime (filepath);
  if (mime_type == "") {

    // File type is not served according to configuration of server.
    request()->write_error_response (x, 403);
    return;
  }

  // Building our standard response headers for a file transfer.
  collection headers {
    {"Content-Type", mime_type},
    {"Content-Length", boost::lexical_cast<string> (size)}};

  // Checking if caller wants to add "Las-Modified" header to envelope.
  if (last_modified)
    headers.push_back ({"Last-Modified", date::from_path_change (filepath).to_string ()});

  // Writing special handler headers to connection.
  write_headers (headers, x, [this, on_success] (auto x) {

    // Invoking on_success() supplied by caller.
    on_success (x);
  });
}


void request_file_handler::write_file (path filepath, unsigned int status_code, bool last_modified, exceptional_executor x, functor on_success)
{
  // Making things slightly more tidy in here.
  using namespace std;

  // Retrieving MIME type, and verifying this is a type of file we actually serve.
  string mime_type = get_mime (filepath);
  if (mime_type == "") {

    // File type is not served according to configuration of server.
    request()->write_error_response (x, 403);
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


void request_file_handler::write_file (path filepath, unsigned int status_code, collection headers, exceptional_executor x, functor on_success)
{
  // Making things slightly more tidy in here.
  using namespace std;

  // Retrieving MIME type, and verifying this is a type of file we actually serve.
  string mime_type = get_mime (filepath);
  if (mime_type == "") {

    // File type is not served according to configuration of server.
    request()->write_error_response (x, 403);
    return;
  }

  // Writing status code.
  write_status (status_code, x, [this, filepath, headers, on_success] (auto x) {

    // Writing special file headers back to client.
    write_file_headers (filepath, false, x, [this, filepath, headers, on_success] (auto x) {

      // Writing extra headers.
      write_headers (headers, x, [this, filepath, on_success] (auto x) {

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
  });
}


void request_file_handler::write_file (shared_ptr<std::ifstream> fs_ptr, exceptional_executor x, functor on_success)
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
    connection()->socket().async_write (bf, [this, on_success, x, fs_ptr] (auto error, auto bytes_written) {

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
