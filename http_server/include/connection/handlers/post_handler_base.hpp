
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

#ifndef ROSETTA_SERVER_POST_HANDLER_BASE_HPP
#define ROSETTA_SERVER_POST_HANDLER_BASE_HPP

#include <tuple>
#include <string>
#include <vector>
#include "common/include/exceptional_executor.hpp"
#include "http_server/include/connection/handlers/content_request_handler.hpp"

namespace rosetta {
namespace http_server {

using std::tuple;
using std::vector;
using std::string;
using namespace rosetta::common;

class request;
class connection;


/// POST authorization data handler.
class post_handler_base : public content_request_handler
{
public:

  /// Creates a POST handler for user data.
  post_handler_base (class request * request);

  /// Handles the given request.
  virtual void handle (connection_ptr connection, std::function<void()> on_success) override;

protected:

  /// POST parameters key/value.
  vector<tuple<string, string>> _parameters;
};


} // namespace http_server
} // namespace rosetta

#endif // ROSETTA_SERVER_POST_HANDLER_BASE_HPP
