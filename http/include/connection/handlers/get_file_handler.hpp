
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

#ifndef ROSETTA_SERVER_STATIC_FILE_HANDLER_HPP
#define ROSETTA_SERVER_STATIC_FILE_HANDLER_HPP

#include "common/include/exceptional_executor.hpp"
#include "http/include/connection/handlers/request_handler_base.hpp"

namespace rosetta {
namespace http {

using std::string;
using namespace rosetta::common;

class request;
class connection;


/// Handles an static file HTTP request.
class get_file_handler final : public request_handler
{
public:

  /// Creates a static file handler.
  get_file_handler (class connection * connection, class request * request);

  /// Handles the given request.
  virtual void handle (exceptional_executor x, functor on_success) override;

private:

  /// Checks if file should be rendered back to client, or if we should return a 304.
  bool should_write_file (path full_path);

  /// Writes 304 response back to client.
  void write_304_response (exceptional_executor x, functor on_success);
};


} // namespace http
} // namespace rosetta

#endif // ROSETTA_SERVER_STATIC_FILE_HANDLER_HPP
