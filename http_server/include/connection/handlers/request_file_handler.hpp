
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

#ifndef ROSETTA_SERVER_REQUEST_FILE_HANDLER_HPP
#define ROSETTA_SERVER_REQUEST_FILE_HANDLER_HPP

#include <fstream>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include "common/include/exceptional_executor.hpp"
#include "http_server/include/connection/handlers/request_handler_base.hpp"

namespace rosetta {
namespace http_server {

using std::string;
using std::ifstream;
using std::shared_ptr;
using namespace rosetta::common;
using namespace boost::filesystem;

class request;
class connection;


/// Creates an HTTP handler for writing files back to client.
class request_file_handler : public request_handler_base
{
protected:

  /// Protected constructor.
  request_file_handler (class connection * connection, class request * request);

  /// Writing the given file's HTTP headers on socket back to client.
  void write_file_headers (path file_path, bool last_modified, exceptional_executor x, functor on_success);

  /// Convenience method; Writes the given file on socket back to client, with a status code, default headers for a file,
  /// standard headers for server, and basically everything.
  void write_file (path file_path, unsigned int status_code, bool last_modified, exceptional_executor x, functor on_success);

  /// Returns the MIME type according to file extension.
  string get_mime (path filename);

private:

  /// Implementation of actual file write operation.
  /// Will read _response_buffer.size() from file, and write buffer content to socket, before invoking self, until entire file has been written.
  void write_file (shared_ptr<ifstream> fs_ptr, exceptional_executor x, functor on_success);


  /// Buffer for sending content back to client in chunks.
  std::array<char, 8192> _response_buffer;
};


} // namespace http_server
} // namespace rosetta

#endif // ROSETTA_SERVER_REQUEST_FILE_HANDLER_HPP
