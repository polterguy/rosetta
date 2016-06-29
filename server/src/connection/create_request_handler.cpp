
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

#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include "common/include/date.hpp"
#include "server/include/server.hpp"
#include "server/include/connection/request.hpp"
#include "server/include/connection/connection.hpp"
#include "server/include/exceptions/request_exception.hpp"
#include "server/include/connection/create_request_handler.hpp"

// Including all HTTP handlers we support.
#include "server/include/connection/handlers/request_handler_base.hpp"
#include "server/include/connection/handlers/get_file_handler.hpp"
#include "server/include/connection/handlers/put_file_handler.hpp"
#include "server/include/connection/handlers/delete_file_handler.hpp"
#include "server/include/connection/handlers/meta/head_handler.hpp"
#include "server/include/connection/handlers/meta/error_handler.hpp"
#include "server/include/connection/handlers/meta/trace_handler.hpp"
#include "server/include/connection/handlers/meta/redirect_handler.hpp"

namespace rosetta {
namespace server {

using std::string;
using namespace boost::asio;
using namespace rosetta::common;


bool in_whitelist (const class connection * connection, const class request * request)
{
  // Making things more tidy in here.
  using namespace std;
  using namespace boost::algorithm;

  // Retrieve the User-Agent whitelist, and see if it has something besides "*" ("accept all") as value.
  const string user_agent_whitelist = connection->server()->configuration().get <string> ("user-agent-whitelist", "*");
  if (user_agent_whitelist != "*") {

    // Retrieving User-Agent header from request envelope.
    const string user_agent = request->envelope().header ("User-Agent");
    if (user_agent.size() == 0)
      return false; // No User-Agent string, and whitelist was defined. Refusing request.

    // Whitelist defined, checking if User-Agent string from request contains at least one of its entries.
    vector<string> whitelist_entities;
    split (whitelist_entities, user_agent_whitelist, boost::is_any_of ("|"));
    for (const auto & idx : whitelist_entities) {
      if (user_agent.find (idx) != string::npos)
        return true; // Match in User-Agent for currently iterated whitelist entity.
    }

    // Did not find a match in User-Agent.
    return false;
  } else {

    // No whitelist defined, accepting everything.
    return true;
  }
}


bool in_blacklist (const class connection * connection, const class request * request)
{
  // Making things more tidy in here.
  using namespace std;
  using namespace boost::algorithm;

  // Retrieve the User-Agent blacklist, and see if it has something besides "" (empty) as value.
  const string user_agent_blacklist = connection->server()->configuration().get <string> ("user-agent-blacklist", "");
  if (user_agent_blacklist != "") {

    // Retrieving User-Agent header from request envelope.
    const string user_agent = request->envelope().header ("User-Agent");
    if (user_agent.size() == 0)
      return false; // No User-Agent string, and blacklist was defined. Accepting request.

    // Blacklist defined, checking if User-Agent string from request contains at least one of its entries.
    vector<string> blacklist_entities;
    split (blacklist_entities, user_agent_blacklist, boost::is_any_of ("|"));
    for (const auto & idx : blacklist_entities) {
      if (user_agent.find (idx) != string::npos)
        return true; // Match in User-Agent for currently iterated blacklist entity.
    }

    // Did not find a match in User-Agent.
    return false;
  } else {

    // No blacklist defined, accepting everything.
    return false;
  }
}


bool should_upgrade_insecure_requests (const class connection * connection, const class request * request)
{
  // Checking if current request is secured already.
  if (!connection->is_secure()) {

    // Checking if server is configured to allow for automatic upgrading of insecure requests.
    if (connection->server()->configuration().get <bool> ("upgrade-insecure-requests", false)) {

      // Checking if client prefers SSL sockets.
      if (request->envelope().header ("Upgrade-Insecure-Requests") == "1") {

        // Checking if server is configure with, and has a root certificate and a private key.
        const string & certificate = connection->server()->configuration().get<string> ("ssl-certificate", "");
        const string & key = connection->server()->configuration().get<string> ("ssl-private-key", "");

        // Checking if neither of the above values are empty.
        if (certificate.size() > 0 && key.size() > 0) {

          // Checking if certificate file and key file actually exists on disc.
          if (exists (certificate) && exists (key)) {

            // We can safely upgrade the current request!
            return true;
          }
        }
      }
    }
  }

  // This request should not be upgraded for some reasons.
  return false;
}


request_handler_ptr upgrade_insecure_request (class connection * connection, class request * request)
{
  // Redirecting client to SSL version of same resource, making sure we don't append "default-page" to the Location URI.
  auto request_uri = request->envelope().uri();
  if (request_uri == connection->server()->configuration().get <path> ("default-page", "/index.html"))
    request_uri = "/";

  // Retrieving server address and SSL port, for our "Location" response header.
  const string server_address = connection->server()->configuration().get <string> ("address", "localhost");
  const string ssl_port       = connection->server()->configuration().get <string> ("ssl-port", "443");
  string new_uri              = "https://" + server_address + (ssl_port == "443" ? "" : ":" + ssl_port) + request_uri.string ();

  // Returning Redirect Temporarily, with a "no-store" value for the "Cache-Control" header.
  return request_handler_ptr (new redirect_handler (connection, request, 307, new_uri, true));
}


request_handler_ptr create_trace_handler (class connection * connection, class request * request)
{
  // Checking if TRACE method is allowed according to configuration.
  if (!connection->server()->configuration().get<bool> ("trace-allowed", false)) {

    // Method not allowed.
    return request_handler_ptr (new error_handler (connection, request, 405));
  } else {

    // Creating a TRACE response handler, and returning to caller.
    return request_handler_ptr (new trace_handler (connection, request));
  }
}


request_handler_ptr create_head_handler (class connection * connection, class request * request)
{
  // Checking that path actually exists.
  if (!exists (request->envelope().path()))
    return request_handler_ptr (new error_handler (connection, request, 404)); // No such path.

  // Checking if HEAD method is allowed according to configuration.
  if (!connection->server()->configuration().get<bool> ("head-allowed", false)) {

    // Method not allowed.
    return request_handler_ptr (new error_handler (connection, request, 405));
  } else {

    // Creating a HEAD response handler.
    return request_handler_ptr (new head_handler (connection, request));
  }
}


request_handler_ptr create_get_handler (class connection * connection, class request * request)
{
  // Checking that path actually exists.
  if (!exists (request->envelope().path()))
    return request_handler_ptr (new error_handler (connection, request, 404)); // No such path.

  // Figuring out if user requested a file or a folder.
  if (is_regular_file (request->envelope().path())) {

    // Figuring out handler to use according to request extension, and if document type is served/handled.
    string extension = request->envelope().path().extension().string ();
    string handler   = connection->server()->configuration().get<string> ("handler" + extension, "error");

    // Returning the correct handler to caller.
    if (handler == "get-file-handler") {

      // Static file GET handler.
      return request_handler_ptr (new get_file_handler (connection, request));
    } else {

      // Oops, these types of files are not served or handled.
      return request_handler_ptr (new error_handler (connection, request, 404));
    }
  } else {

    // Oops, these types of files/folders are not served or handled. (yet)
    return request_handler_ptr (new error_handler (connection, request, 404));
  }
}


request_handler_ptr create_put_handler (class connection * connection, class request * request)
{
  // Checking that folder where user tries to put file actually exists.
  if (!exists (request->envelope().path().parent_path()))
    return request_handler_ptr (new error_handler (connection, request, 404)); // No such folder.

  // Figuring out if user requested a file or a folder.
  if (is_regular_file (request->envelope().path())) {

    // User tries to PUT a file.
    return request_handler_ptr (new put_file_handler (connection, request));
  } else {

    // Oops, no such PUT handler. (yet)
    return request_handler_ptr (new error_handler (connection, request, 403));
  }
}


request_handler_ptr create_delete_handler (class connection * connection, class request * request)
{
  // Checking that path actually exists.
  if (!exists (request->envelope().path()))
    return request_handler_ptr (new error_handler (connection, request, 404)); // No such path.

  // Figuring out if user requested a file or a folder.
  if (is_regular_file (request->envelope().path ())) {

    // User tries to DELETE a file.
    return request_handler_ptr (new delete_file_handler (connection, request));
  } else {

    // Oops, no such DELETE handler. (yet)
    return request_handler_ptr (new error_handler (connection, request, 403));
  }
}


request_handler_ptr create_request_handler (class connection * connection, class request * request, int status_code)
{
  // Checking if we can accept User-Agent according whitelist and blacklist definitions.
  if (!in_whitelist (connection, request) || in_blacklist (connection, request)) {

    // User-Agent not accepted!
    return request_handler_ptr (new error_handler (connection, request, 403));
  }

  // Checking request type, and other parameters, deciding which type of request handler we should create.
  if (status_code >= 400) {

    // Some sort of error.
    return request_handler_ptr (new error_handler (connection, request, status_code));

  } else if (should_upgrade_insecure_requests (connection, request)) {

    // Both configuration, and client, prefers secure requests, and current connection is not secure, hence we upgrade.
    return upgrade_insecure_request (connection, request);

  } else if (request->envelope().method() == "TRACE") {

    // Returning a TRACE handler.
    return create_trace_handler (connection, request);

  } else if (request->envelope().method() == "HEAD") {

    // Returning a HEAD handler.
    return create_head_handler (connection, request);

  } else if (request->envelope().method() == "GET") {

    // Returning a GET file handler.
    return create_get_handler (connection, request);

  } else if (request->envelope().method() == "PUT") {

    // Returning a GET file handler.
    return create_put_handler (connection, request);

  } else if (request->envelope().method() == "DELETE") {

    // Returning a GET file handler.
    return create_delete_handler (connection, request);

  } else {

    // Unsupported method.
    return request_handler_ptr (new error_handler (connection, request, 405));
  }
}


} // namespace server
} // namespace rosetta
