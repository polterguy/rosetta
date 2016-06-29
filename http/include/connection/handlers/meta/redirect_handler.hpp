
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

#ifndef ROSETTA_SERVER_REDIRECT_HANDLER_HPP
#define ROSETTA_SERVER_REDIRECT_HANDLER_HPP

#include "common/include/exceptional_executor.hpp"
#include "http/include/connection/handlers/request_handler_base.hpp"

namespace rosetta {
namespace http {

using std::string;
using namespace rosetta::common;

class request;
class connection;


/// Handles an HTTP request.
class redirect_handler final : public request_handler
{
public:

  /// Creates a redirect file handler.
  redirect_handler (class connection * connection, class request * request, unsigned int status, const string & uri, bool no_store);

  /// Handles the given request.
  virtual void handle (exceptional_executor x, functor on_success) override;

private:

  /// The HTTP status code of the response we should serve.
  const unsigned int _status;

  /// The URI of the redirect response.
  const string _uri;

  /// If true, then make sure we return a "Cache-Control" header, with a "no-store" value to client.
  const bool _no_store;
};


} // namespace http
} // namespace rosetta

#endif // ROSETTA_SERVER_REDIRECT_HANDLER_HPP
