
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

#include "server/include/server.hpp"
#include "server/include/connection/request.hpp"
#include "server/include/connection/connection.hpp"
#include "server/include/exceptions/server_exception.hpp"
#include "server/include/exceptions/request_exception.hpp"
#include "server/include/connection/handlers/request_handler.hpp"

namespace rosetta {
namespace server {
  
using std::string;
using boost::system::error_code;
using namespace rosetta::common;


request_ptr request::create (connection * connection)
{
  return std::shared_ptr<request> (new request (connection));
}


request::request (connection * connection)
  : _connection (connection),
    _envelope (connection, this)
{ }


// Notice, if this method is successful, through all layers, it will invoke the given on_success() function, passing in
// the "x" parameter. If not, then "x", which is an exceptional_executor, will be executed, to make sure we
// clean up things, if some exception, etc, occurs. This creates a guarantee of that connection will be closed,
// if any of the layers, from this point and inwards, raises an exception. Since the "x" parameter is passed into
// all layers, with its copy semantics, that mimics the old auto_ptr class from C++, from this point and inwards,
// we have a guarantee of that "x" will be executed, unless we reach the point where on_success() can be safely invoked.
// This lets the method that invokes this method decide what to do, both if an exception occurs, and if success is reached.
void request::handle (exceptional_executor x, functor on_success)
{
  // Reading envelope, passing in reference to "self"
  _envelope.read (x, [this, on_success] (auto x) {

    // Settings deadline timer.
    const int CONTENT_READ_TIMEOUT = _connection->server()->configuration().get<size_t> ("request-content-read-timeout", 300);
    _connection->set_deadline_timer (CONTENT_READ_TIMEOUT);

    // Now reading is done, and we can let our request_handler take care of the rest.
    _request_handler = request_handler::create (_connection, this);
    if (_request_handler == nullptr)
      return; // No handler for this request, returning without releasing "x", to close connection.

    // Letting request_handler take over from here.
    _request_handler->handle (x, [this, on_success] (auto x) {

      // Request is now finished handled, and we need to determine if we should keep connection alive or not.
      if (_envelope.header ("Connection") != "close") {

        // Connection should be kept alive, releasing exceptional_executor, and invoking on_success() on connection, should do the trick.
        // But first we must do a "force read" of content, unless it has already been read, since otherwise the next request will be malformed,
        // due to content being "in the way".
        ensure_read_content (x, [this, on_success] (auto x) {

          // Invoking on_success callback.
          on_success (x);
        });
      } // else - x goes out of scope, and releases connection, and all resources associated with it ...
    });
  });
}


void request::ensure_read_content (exceptional_executor x, functor callback)
{
  // Checking if we have already read content.
  if (_content_has_been_read)
    return;

  // Making sure we signal to any future invocations that content is already read.
  _content_has_been_read = true;

  // Max allowed length of content.
  const size_t MAX_REQUEST_CONTENT_LENGTH = _connection->server()->configuration().get<size_t> ("max-request-content-length", 4194304);

  // Checking if there is any content first.
  string content_length_str = _envelope.header ("Content-Length");

  // Checking if there is any Content-Length
  if (content_length_str.size() == 0) {

    // No content.
    callback (x);
  } else {

    // Checking that content does not exceed max request content length.
    auto content_length = boost::lexical_cast<size_t> (content_length_str);
    if (content_length > MAX_REQUEST_CONTENT_LENGTH)
      return; // Too much data in content sent from client, according to configuration. Simply letting x go out of scope, cleans everything up.

    // Reading content into streambuf, and keeping it there, for any request_handlers that might have interest in it, for some reasons.
    _connection->socket().async_read (_connection->buffer(), transfer_exactly (content_length), [x, callback] (auto error, auto bytes_read) {

      // Checking for socket errors.
      if (error)
        throw request_exception ("Socket error while reading request content.");

      // Invoking functor callback supplied by caller.
      callback (x);
    });
  }
}


void request::write_error_response (exceptional_executor x, int status_code)
{
  // Verify that this actually is an error, and if not, throws an exception.
  if (status_code < 400)
    throw server_exception ("Logical error in server. Tried to return a non-error status as an error.");

  // Creating an error handler.
  _request_handler = request_handler::create (_connection, this, status_code);
  if (_request_handler == nullptr)
    throw server_exception ("Error handler not found for status code; '" + boost::lexical_cast<string> (status_code) + "'.");

  // Letting error handler take over from here.
  _request_handler->handle (x, [this] (auto x) {

    // Simply letting x go out of scope, to close down connection, and clean things up.
    // For security reasons, we do not let connection stay alive, if client creates an error response.
  });
}


} // namespace server
} // namespace rosetta
