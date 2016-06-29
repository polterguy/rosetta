
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

#include "common/include/exceptional_executor.hpp"
#include "server/include/connection/handlers/request_handler_base.hpp"

namespace rosetta {
namespace server {

using std::string;
using namespace rosetta::common;

class request;
class connection;
const static size_t BUFFER_SIZE = 8192;


/// Handles an HTTP request.
class put_file_handler final : public request_handler
{
public:

  /// Creates a static file handler.
  put_file_handler (class connection * connection, class request * request);

  /// Handles the given request.
  virtual void handle (exceptional_executor x, functor on_success) override;

private:

  /// Saves content of request to the specified file.
  void save_request_content (const string & filename, exceptional_executor x, functor on_success);

  /// Write request content to file.
  void save_request_content_to_file (std::shared_ptr<std::ofstream> file_ptr,
                                     std::shared_ptr<std::istream> socket_stream_ptr,
                                     size_t content_length,
                                     exceptional_executor x,
                                     exceptional_executor x2,
                                     functor on_success);

  /// Returns Content-Length of request, and verifies there is any content, and that request is not malformed.
  size_t get_content_length ();

  /// Writes success return to client.
  void write_success_envelope (exceptional_executor x, functor on_success);


  /// Buffer used for reading content from socket.
  std::array<char, BUFFER_SIZE> _file_buffer;
};


} // namespace server
} // namespace rosetta

#endif // ROSETTA_SERVER_PUT_FILE_HANDLER_HPP
