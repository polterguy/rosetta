
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

#ifndef ROSETTA_SERVER_REQUEST_HANDLER_HPP
#define ROSETTA_SERVER_REQUEST_HANDLER_HPP

#include <tuple>
#include <vector>
#include "common/include/exceptional_executor.hpp"

namespace rosetta {
namespace http_server {

using std::string;
using namespace rosetta::common;

class request;
class connection;

// Helpers for HTTP headers.
typedef std::tuple<string, string> collection_type;
typedef std::vector<collection_type> collection;


/// Handles an HTTP request.
class request_handler_base : public boost::noncopyable
{
public:

  /// Handles the given request.
  virtual void handle (exceptional_executor x, functor callback) = 0;

protected:

  /// Protected constructor.
  request_handler_base (class connection * connection, class request * request);

  /// Writing given HTTP headetr with given value back to client.
  void write_status (unsigned int status_code, exceptional_executor x, functor on_success);

  /// Writes a single HTTP header, with the given name/value combination back to client.
  void write_header (const string & key, const string & value, exceptional_executor x, functor on_success);

  /// Writing given HTTP header collection back to client.
  void write_headers (collection headers, exceptional_executor x, functor on_success);

  /// Writes back the standard HTTP headers back to client, that the server is configured to pass back on every response.
  void write_standard_headers (exceptional_executor x, functor on_success);

  /// Ensures that the envelope of the response is flushed, and one empty line with CR/LF is written back to the client.
  void ensure_envelope_finished (exceptional_executor x, functor on_success);

  /// Returns connection for this instance.
  connection * connection() { return _connection; }

  /// Returns request for this instance.
  request * request() { return _request; }

private:

  /// The connection this instance belongs to.
  class connection * _connection;

  /// The request that owns this instance.
  class request * _request;
};


} // namespace http_server
} // namespace rosetta

#endif // ROSETTA_SERVER_REQUEST_HANDLER_HPP
