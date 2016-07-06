
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

#ifndef ROSETTA_SERVER_DELETE_HANDLER_HPP
#define ROSETTA_SERVER_DELETE_HANDLER_HPP

#include "common/include/exceptional_executor.hpp"
#include "http_server/include/connection/handlers/request_handler_base.hpp"

namespace rosetta {
namespace http_server {

using std::string;
using namespace rosetta::common;

class request;
class connection;


/// DELETE handler for static files.
class delete_handler final : public request_handler_base
{
public:

  /// Creates a static file handler.
  delete_handler (class connection * connection, class request * request);

  /// Handles the given request.
  virtual void handle (std::function<void()> on_success) override;

private:

  /// Writes success return to client.
  void write_success_envelope (std::function<void()> on_success);
};


} // namespace http_server
} // namespace rosetta

#endif // ROSETTA_SERVER_DELETE_HANDLER_HPP
