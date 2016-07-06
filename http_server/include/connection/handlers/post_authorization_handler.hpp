
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

#ifndef ROSETTA_SERVER_POST_AUTHORIZATION_HANDLER_HPP
#define ROSETTA_SERVER_POST_AUTHORIZATION_HANDLER_HPP

#include "common/include/exceptional_executor.hpp"
#include "http_server/include/connection/handlers/post_handler_base.hpp"

namespace rosetta {
namespace http_server {

using namespace rosetta::common;

class request;
class connection;


/// POST authorization data handler.
class post_authorization_handler final : public post_handler_base
{
public:

  /// Creates a POST handler for user data.
  post_authorization_handler (class connection * connection, class request * request);

  /// Handles the given request.
  virtual void handle (std::function<void()> on_success) override;

private:

  /// Evaluates request after parsing is done.
  void evaluate ();
};


} // namespace http_server
} // namespace rosetta

#endif // ROSETTA_SERVER_POST_AUTHORIZATION_HANDLER_HPP
