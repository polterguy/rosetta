
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

#ifndef ROSETTA_SERVER_PUT_FILE_HANDLER_HPP
#define ROSETTA_SERVER_PUT_FILE_HANDLER_HPP

#include <array>
#include <fstream>
#include <iostream>
#include "common/include/exceptional_executor.hpp"
#include "http_server/include/connection/handlers/request_handler_base.hpp"

using std::array;
using std::string;
using std::istream;
using std::shared_ptr;
using namespace rosetta::common;

namespace rosetta {
namespace http_server {

class request;
class connection;
const static size_t BUFFER_SIZE = 8192;


/// PUT handler for static files.
class put_file_handler final : public request_handler_base
{
public:

  /// Creates a PUT handler.
  put_file_handler (connection_ptr connection, class request * request);

  /// Handles the given request.
  virtual void handle (std::function<void()> on_success) override;

private:

  /// Saves content of request to the specified file.
  void save_request_content (path filename, std::function<void()> on_success);

  /// Write request content to file.
  void save_request_content_to_file (shared_ptr<std::ofstream> file_ptr,
                                     shared_ptr<istream> socket_stream_ptr,
                                     size_t content_length,
                                     exceptional_executor x,
                                     std::function<void()> on_success);

  /// Returns Content-Length of request, and verifies there is any content, and that request is not malformed.
  size_t get_content_length ();

  /// Writes success return to client.
  void write_success_envelope (std::function<void()> on_success);


  /// Buffer used for reading content from socket.
  array<char, BUFFER_SIZE> _file_buffer;
};


} // namespace http_server
} // namespace rosetta

#endif // ROSETTA_SERVER_PUT_FILE_HANDLER_HPP
