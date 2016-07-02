
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
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "http_server/include/server.hpp"
#include "http_server/include/helpers/date.hpp"
#include "http_server/include/connection/request.hpp"
#include "http_server/include/connection/connection.hpp"
#include "http_server/include/exceptions/request_exception.hpp"
#include "http_server/include/exceptions/security_exception.hpp"
#include "http_server/include/connection/create_request_handler.hpp"

// Including all HTTP handlers we support.
#include "http_server/include/connection/handlers/request_handler_base.hpp"
#include "http_server/include/connection/handlers/request_file_handler.hpp"
#include "http_server/include/connection/handlers/get_file_handler.hpp"
#include "http_server/include/connection/handlers/get_folder_handler.hpp"
#include "http_server/include/connection/handlers/put_file_handler.hpp"
#include "http_server/include/connection/handlers/put_folder_handler.hpp"
#include "http_server/include/connection/handlers/delete_handler.hpp"
#include "http_server/include/connection/handlers/meta/head_handler.hpp"
#include "http_server/include/connection/handlers/meta/error_handler.hpp"
#include "http_server/include/connection/handlers/meta/trace_handler.hpp"
#include "http_server/include/connection/handlers/meta/redirect_handler.hpp"
#include "http_server/include/connection/handlers/meta/unauthorized_handler.hpp"

namespace rosetta {
namespace http_server {

using std::string;
using namespace boost::asio;
using namespace boost::filesystem;
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
  // Redirecting client to SSL version of same resource.
  auto request_uri = request->envelope().uri();

  // Retrieving server address and SSL port, for our "Location" response header.
  const string server_address = connection->server()->configuration().get <string> ("address", "localhost");
  const string ssl_port       = connection->server()->configuration().get <string> ("ssl-port", "443");
  string new_uri              = "https://" + server_address + (ssl_port == "443" ? "" : ":" + ssl_port) + request_uri.string ();

  // Looping through all parameters, adding these to the Location URI.
  bool first = true;
  for (auto & idx : request->envelope().parameters()) {
    if (first) {
      first = false;
      new_uri += "?";
    } else {
      new_uri += "&";
    }
    new_uri += std::get<0> (idx);
    auto val = std::get<1> (idx);
    if (val.size() > 0)
      new_uri += "=" + val;
  }

  // Returning Redirect Temporarily, with a "no-store" value for the "Cache-Control" header.
  return request_handler_ptr (new redirect_handler (connection, request, 307, new_uri, true));
}


bool authorize_request (connection * connection, request * request)
{
  auto ticket = request->envelope().ticket();
  auto path = request->envelope().path();
  auto method = request->envelope().method();
  return connection->server()->authorization().authorize (ticket, path, method);
}


request_handler_ptr create_trace_handler (class connection * connection, class request * request)
{
  // Authorizing request.
  if (!authorize_request (connection, request))
    return request_handler_ptr (new unauthorized_handler (connection, request, !request->envelope().ticket().authenticated()));

  // Creating a TRACE response handler, and returning to caller.
  return request_handler_ptr (new trace_handler (connection, request));
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

    // Authorizing request.
    if (!authorize_request (connection, request))
      return request_handler_ptr (new unauthorized_handler (connection, request, !request->envelope().ticket().authenticated()));

    // Creating a HEAD response handler.
    return request_handler_ptr (new head_handler (connection, request));
  }
}


request_handler_ptr create_get_handler (class connection * connection, class request * request)
{
  // Checking that path actually exists.
  if (!exists (request->envelope().path()))
    return request_handler_ptr (new error_handler (connection, request, 404)); // No such path.

  // Authorizing request.
  if (!authorize_request (connection, request))
    return request_handler_ptr (new unauthorized_handler (connection, request, !request->envelope().ticket().authenticated()));

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

    // This is a request for a "folder".
    // Notice, if the client sends an "authorize" parameter, we force the "authorized" version of the folder view.
    if (!request->envelope().ticket().authenticated() && request->envelope().has_parameter ("authorize"))
      return request_handler_ptr (new unauthorized_handler (connection, request, true));
    return request_handler_ptr (new get_folder_handler (connection, request));
  }
}


request_handler_ptr create_put_handler (class connection * connection, class request * request)
{
  // Checking that folder where user tries to put file/folder actually exists.
  if (!exists (request->envelope().path().parent_path()))
    return request_handler_ptr (new error_handler (connection, request, 404)); // No such folder.

  // Authorizing request.
  if (!authorize_request (connection, request))
    return request_handler_ptr (new unauthorized_handler (connection, request, !request->envelope().ticket().authenticated()));

  // Figuring out if client wants to PUT a file or a folder.
  if (request->envelope().file_request()) {

    // User tries to PUT a file.
    // Figuring out if file exists from before, and if it does, verify that user is authorized to DELETE file,
    // since this effectively is a "DELETE" operation, due to overwriting an existing file, deletes its old content.
    if (exists (request->envelope().path()))
      if (!connection->server()->authorization().authorize (request->envelope().ticket(), request->envelope().path(), "DELETE"))
        return request_handler_ptr (new unauthorized_handler (connection, request, !request->envelope().ticket().authenticated()));

    return request_handler_ptr (new put_file_handler (connection, request));
  } else {

    // User tries to put a folder.
    // Figuring out if folder exists from before, and if it does, we deny client to PUT the folder.
    if (exists (request->envelope().path()))
      return request_handler_ptr (new unauthorized_handler (connection, request, false));
    return request_handler_ptr (new put_folder_handler (connection, request));
  }
}


request_handler_ptr create_delete_handler (class connection * connection, class request * request)
{
  // Checking that path actually exists.
  if (!exists (request->envelope().path()))
    return request_handler_ptr (new error_handler (connection, request, 404)); // No such path.

  // Checking if client is authorized to using DELETE verb towards path.
  if (!connection->server()->authorization().authorize (request->envelope().ticket(), request->envelope().path(), "DELETE"))
    return request_handler_ptr (new unauthorized_handler (connection, request, !request->envelope().ticket().authenticated()));

  // User tries to DELETE a file or a folder.
  return request_handler_ptr (new delete_handler (connection, request));
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
  }

  if (should_upgrade_insecure_requests (connection, request)) {

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


} // namespace http_server
} // namespace rosetta
