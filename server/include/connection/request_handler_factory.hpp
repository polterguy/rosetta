
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

#ifndef ROSETTA_SERVER_REQUEST_HANDLER_FACTORY_HPP
#define ROSETTA_SERVER_REQUEST_HANDLER_FACTORY_HPP

#include <tuple>
#include <memory>
#include <vector>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include "common/include/exceptional_executor.hpp"

namespace rosetta {
namespace server {

using std::string;
using namespace rosetta::common;
using namespace boost::filesystem;

class request;
class connection;
class request_handler;
typedef std::shared_ptr<request_handler> request_handler_ptr;


/// Creates a request_handler.
class request_handler_factory : public boost::noncopyable
{
public:

  /// Creates the specified type of handler, according to file extension given, and configuration of server.
  static request_handler_ptr create (connection * connection, request * request, int status_code = -1);

private:

  /// To prevent instances from being created.
  request_handler_factory ();

  /// Returns true if User-Agent is in white list.
  static bool in_whitelist (const class connection * connection, const class request * request);

  /// Returns true if User-Agent is in black list.
  static bool in_blacklist (const class connection * connection, const class request * request);

  /// Returns true if we should upgrade request to a secure SSL connection.
  static bool should_upgrade_insecure_requests (const class connection * connection, const class request * request);

  // Factory methods.

  /// Returns a request_handler for upgrading an insecure request. 307 temporary redirection handler.
  static request_handler_ptr upgrade_insecure_request (class connection * connection, class request * request);

  /// Returns a TRACE HTTP request_handler.
  static request_handler_ptr create_trace_handler (class connection * connection, class request * request);

  /// Returns a HEAD HTTP request_handler.
  static request_handler_ptr create_head_handler (class connection * connection, class request * request);

  /// Returns a GET HTTP request_handler.
  static request_handler_ptr create_get_handler (class connection * connection, class request * request);

  /// Returns a PUT HTTP request_handler.
  static request_handler_ptr create_put_handler (class connection * connection, class request * request);

  /// Returns a DELETE HTTP request_handler.
  static request_handler_ptr create_delete_handler (class connection * connection, class request * request);
};


} // namespace server
} // namespace rosetta

#endif // ROSETTA_SERVER_REQUEST_HANDLER_FACTORY_HPP
