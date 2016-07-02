
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
#include "http_server/include/connection/handlers/request_handler_base.hpp"

namespace rosetta {
namespace http_server {

using std::tuple;
using std::vector;
using std::string;
using namespace rosetta::common;

class request;
class connection;


/// POST authorization data handler.
class post_handler_base : public request_handler_base
{
public:

  /// Creates a POST handler for user data.
  post_handler_base (class connection * connection, class request * request);

  /// Handles the given request.
  virtual void handle (exceptional_executor x, functor on_success) override;

protected:

  /// Writes success return to client.
  void write_success_envelope (exceptional_executor x, functor on_success);

  /// POST parameters key/value.
  vector<tuple<string, string>> _parameters;

private:

  /// Validates and returns the length of the request.
  size_t get_content_length ();
};


} // namespace http_server
} // namespace rosetta

#endif // ROSETTA_SERVER_POST_HANDLER_BASE_HPP
