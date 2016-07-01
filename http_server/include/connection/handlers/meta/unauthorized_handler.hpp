
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

#ifndef ROSETTA_SERVER_UNAUTHORIZED_HANDLER_HPP
#define ROSETTA_SERVER_UNAUTHORIZED_HANDLER_HPP

#include "common/include/exceptional_executor.hpp"
#include "http_server/include/connection/handlers/meta/error_handler.hpp"

using namespace rosetta::common;

namespace rosetta {
namespace http_server {

class request;
class connection;


/// Unauthorized handler.
class unauthorized_handler final : public error_handler
{
public:

  /// Creates an error request handler.
  unauthorized_handler (class connection * connection, class request * request, bool allow_authentication);

  /// Handles the given request.
  virtual void handle (exceptional_executor x, functor on_success) override;

private:

  /// Whether or not authorization of client should be allowed.
  const bool _allow_authentication;
};


} // namespace http_server
} // namespace rosetta

#endif // ROSETTA_SERVER_UNAUTHORIZED_HANDLER_HPP
