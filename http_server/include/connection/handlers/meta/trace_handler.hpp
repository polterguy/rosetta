
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

#ifndef ROSETTA_SERVER_TRACE_HANDLER_HPP
#define ROSETTA_SERVER_TRACE_HANDLER_HPP

#include <string>
#include "common/include/exceptional_executor.hpp"
#include "http_server/include/connection/handlers/request_handler_base.hpp"

namespace rosetta {
namespace http_server {

class request;
class connection;


/// Echoes the HTTP-Request line and the request headers from the request back to the client as text/plain content.
/// Notice, this will return the envelope of the request as Rosetta sees it, after having intelligently parsed it,
/// and not necessarily exactly as the client sent the request. Except for the URI of the HTTP-Request line, that
/// will be URI encoded.
class trace_handler final : public request_handler
{
public:

  /// Creates a trace handler.
  trace_handler (class connection * connection, class request * request);

  /// Handles the given request.
  virtual void handle (exceptional_executor x, functor on_success) override;

private:

  /// Builds up the buffer for what to return as content to client.
  std::shared_ptr<std::vector<unsigned char> > build_content ();
};


} // namespace http_server
} // namespace rosetta

#endif // ROSETTA_SERVER_TRACE_HANDLER_HPP
