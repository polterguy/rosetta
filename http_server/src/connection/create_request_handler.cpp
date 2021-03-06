
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
#include "http_server/include/helpers/uri_encode.hpp"
#include "http_server/include/connection/request.hpp"
#include "http_server/include/connection/connection.hpp"
#include "http_server/include/exceptions/request_exception.hpp"
#include "http_server/include/connection/create_request_handler.hpp"

// Including all HTTP handlers we support.
#include "http_server/include/connection/handlers/request_handler_base.hpp"
#include "http_server/include/connection/handlers/request_file_handler.hpp"
#include "http_server/include/connection/handlers/get_file_handler.hpp"
#include "http_server/include/connection/handlers/get_folder_handler.hpp"
#include "http_server/include/connection/handlers/put_file_handler.hpp"
#include "http_server/include/connection/handlers/put_folder_handler.hpp"
#include "http_server/include/connection/handlers/delete_handler.hpp"
#include "http_server/include/connection/handlers/post_users_handler.hpp"
#include "http_server/include/connection/handlers/post_authorization_handler.hpp"

// Meta HTTP handlers.
#include "http_server/include/connection/handlers/meta/head_handler.hpp"
#include "http_server/include/connection/handlers/meta/options_handler.hpp"
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


bool in_user_agent_list (connection_ptr connection, const class request * request, const string & list)
{
  // Making things more tidy in here.
  using namespace std;
  using namespace boost::algorithm;

  // Retrieve the specified list from our configuration file.
  const string configuration_list = connection->server()->configuration().get <string> ("user-agent-" + list, "*");
  if (configuration_list == "*") {

    // List matches everything.
    return true;
  } else if (configuration_list == "") {

    // List matches nothing.
    return false;
  } else {

    // Retrieving User-Agent header from request envelope.
    const string & user_agent = request->envelope().header ("User-Agent");
    if (user_agent.size() == 0)
      return false;

    // Checking if User-Agent string from client's request contains at least one of our list entries.
    vector<string> list_entries;
    split (list_entries, configuration_list, boost::is_any_of ("|"));
    for (const auto & idx : list_entries) {
      if (user_agent.find (idx) != string::npos)
        return true; // Match in User-Agent for currently iterated list entity.
    }

    // Did not find a match in User-Agent.
    return false;
  }
}


bool in_user_agent_whitelist (connection_ptr connection, const class request * request)
{
  return in_user_agent_list (connection, request, "whitelist");
}


bool in_user_agent_blacklist (connection_ptr connection, const class request * request)
{
  return in_user_agent_list (connection, request, "blacklist");
}


bool should_upgrade_insecure_requests (connection_ptr connection, const class request * request)
{
  // Checking if current request is secured already.
  if (!connection->is_secure()) {

    // Checking if server is configured to allow for automatic upgrading of insecure requests.
    if (connection->server()->configuration().get <bool> ("upgrade-insecure-requests", true)) {

      // Checking if client prefers SSL sockets.
      if (request->envelope().header ("Upgrade-Insecure-Requests") == "1") {

        // Checking if server is configured with key/certificate, and that certificate/private-key exists.
        const string & certificate = connection->server()->configuration().get<string> ("ssl-certificate", "server.crt");
        const string & key = connection->server()->configuration().get<string> ("ssl-private-key", "server.key");

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


request_handler_ptr upgrade_insecure_request (connection_ptr connection, class request * request)
{
  // Redirecting client to SSL version of the same resource.
  auto request_uri = request->envelope().uri().string ();

  // Retrieving server address and SSL port, for our "Location" response header.
  const string server_address = connection->server()->configuration().get <string> ("address", "localhost");
  const string ssl_port       = connection->server()->configuration().get <string> ("ssl-port", "8081");
  string new_uri              = "https://" + server_address + (ssl_port == "443" ? "" : ":" + ssl_port) + request_uri;

  // Looping through all parameters, adding these to the Location URI.
  bool first = true;
  for (auto & idx : request->envelope().parameters()) {
    if (first) {
      first = false;
      new_uri += "?";
    } else {
      new_uri += "&";
    }
    new_uri += uri_encode::encode (std::get<0> (idx));
    auto val = std::get<1> (idx);
    if (val.size() > 0)
      new_uri += "=" + uri_encode::encode (val);
  }

  // Returning Redirect Temporarily, with a "no-store" value for the "Cache-Control" header.
  return request_handler_ptr (new redirect_handler (request, 307, new_uri, true));
}


bool authorize_request (connection_ptr connection, request * request)
{
  auto ticket = request->envelope().ticket();
  auto path = request->envelope().path();
  auto method = request->envelope().method();

  if (method == "PUT") {

    // A PUT operation, for something that exists from before, is actually also a DELETE operation, since it overwrites existing content.
    if (exists (path)) {

      if (is_directory (path)) {

        // Client cannot "overwrite" an existing directory.
        return false;
      } else {
        
        // If client tries to PUT a file that already exists, then this is also ipso facto a DELETE, hence authorizing for both in such cases.
        if (!connection->server()->authorization().authorize (ticket, path, "DELETE"))
          return false;
        return connection->server()->authorization().authorize (ticket, path, "PUT");
      }
    } else {

      // Path does not exist.
      return connection->server()->authorization().authorize (ticket, path, "PUT");
    }
  } else {

    // Not a PUT request.
    return connection->server()->authorization().authorize (ticket, path, method);
  }
}


request_handler_ptr create_authorize_handler (connection_ptr connection, class request * request)
{
  return request_handler_ptr (new unauthorized_handler (request, !request->envelope().ticket().authenticated()));
}


request_handler_ptr create_trace_handler (connection_ptr connection, class request * request)
{
  // Authorizing request.
  if (authorize_request (connection, request)) {

    // Checking if TRACE method is allowed according to configuration.
    if (!connection->server()->configuration().get<bool> ("trace-allowed", false)) {

      // Method not allowed.
      return request_handler_ptr (new error_handler (request, 405));
    } else {

      // Creating a TRACE response handler, and returning to caller.
      return request_handler_ptr (new trace_handler (request));
    }
  } else {

    // Not authorized.
    return create_authorize_handler (connection, request);
  }
}


request_handler_ptr create_head_handler (connection_ptr connection, class request * request)
{
  // Authorizing request.
  if (authorize_request (connection, request)) {

    // Checking if HEAD method is allowed according to configuration.
    if (!connection->server()->configuration().get<bool> ("head-allowed", false)) {

      // Method not allowed.
      return request_handler_ptr (new error_handler (request, 405));
    } else {

      // Checking that path actually exists.
      if (!exists (request->envelope().path()))
        return request_handler_ptr (new error_handler (request, 404)); // No such path.
      else
        return request_handler_ptr (new head_handler (request));
    }
  } else {

    // Not authorized.
    return create_authorize_handler (connection, request);
  }
}


request_handler_ptr create_options_handler (connection_ptr connection, class request * request)
{
  // Authorizing request.
  if (authorize_request (connection, request)) {

    // Checking if OPTIONS method is allowed according to configuration.
    if (!connection->server()->configuration().get<bool> ("options-allowed", false)) {

      // Method not allowed.
      return request_handler_ptr (new error_handler (request, 405));
    } else {

      // Creating an OPTIONS response handler, and returning to caller.
      return request_handler_ptr (new options_handler (request));
    }
  } else {

    // Not authorized.
    return create_authorize_handler (connection, request);
  }
}


request_handler_ptr create_get_file_handler (connection_ptr connection, class request * request)
{
  // Figuring out handler to use according to request extension, and if document type is served/handled.
  string extension = request->envelope().path().extension().string ();
  string handler   = connection->server()->configuration().get<string> ("handler" + extension, "error");

  // Returning the correct handler to caller.
  if (handler == "get-file-handler") {

    // Static file GET handler.
    return request_handler_ptr (new get_file_handler (request));
  } else {

    // Oops, these types of files are not served or handled.
    return request_handler_ptr (new error_handler (request, 404));
  }
}


request_handler_ptr create_get_handler (connection_ptr connection, class request * request)
{
  // Authorizing request.
  if (authorize_request (connection, request)) {

    // Checking that path actually exists.
    if (!exists (request->envelope().path())) {

      // No such path.
      return request_handler_ptr (new error_handler (request, 404));
    } else {

      // Figuring out if user requested a file or a folder.
      if (is_regular_file (request->envelope().path()) && request->envelope().file_request()) {

        // Returning the correct file handler according to configuration
        return create_get_file_handler (connection, request);
      } else if (is_directory (request->envelope().path()) && request->envelope().folder_request()) {

        // This is a request for a folder's content.
        return request_handler_ptr (new get_folder_handler (request));
      } else {

        // User tries to GET something that's neither a folder, nor a file, or a file/folder, as something it is not.
        return request_handler_ptr (new error_handler (request, 404));
      }
    }
  } else {

    // Not authorized.
    return create_authorize_handler (connection, request);
  }
}


request_handler_ptr create_put_handler (connection_ptr connection, class request * request)
{
  // Authorizing request.
  if (authorize_request (connection, request)) {

    // Checking that parent folder of file/folder actually exists.
    if (!exists (request->envelope().path().parent_path())) {

      // Client tries to PUT something to a location that does not exist.
      return request_handler_ptr (new error_handler (request, 404));
    } else {

      // Figuring out if client wants to PUT a file or a folder.
      if (request->envelope().file_request()) {

        // User tries to PUT a file.
        return request_handler_ptr (new put_file_handler (request));
      } else {

        // User tries to PUT a folder.
        return request_handler_ptr (new put_folder_handler (request));
      }
    }
  } else {

    // Not authorized.
    return create_authorize_handler (connection, request);
  }
}


request_handler_ptr create_delete_handler (connection_ptr connection, class request * request)
{
  // Checking if client is authorized to use the DELETE verb towards path.
  if (authorize_request (connection, request)) {

    // Checking that path actually exists.
    if (!exists (request->envelope().path())) {
    
      // No such path.
      return request_handler_ptr (new error_handler (request, 404));
    } else {
    
      // User tries to DELETE a file or a folder.
      return request_handler_ptr (new delete_handler (request));
    }
  } else {

    // Not authorized.
    return create_authorize_handler (connection, request);
  }
}


request_handler_ptr create_post_users_handler (connection_ptr connection, class request * request)
{
  // No need to authorize these types of request, since all authenticated clients are allowed to post to the ".users" file, though
  // only root accounts are allowed to do anything but changing their own password.
  // Therefor we simply check if client is authenticated at all, before creating our POST users handler.
  // Then we verify type of POST action, and authorize the user, inside our post_users_handler.
  if (request->envelope().ticket().authenticated()) {

    // User tries to POST data to server's ".users" file.
    return request_handler_ptr (new post_users_handler (request));
  } else {

    // Not authorized.
    return create_authorize_handler (connection, request);
  }
}


request_handler_ptr create_post_authorization_handler (connection_ptr connection, class request * request)
{
  // No need to authorize these types of request, since only "root" accounts are allowed to post to the ".auth" files at all.
  if (request->envelope().ticket().role == "root") {

    // User tries to POST data to a '.auth' file in some folder.
    return request_handler_ptr (new post_authorization_handler (request));
  } else {

    // Not authenticated.
    return create_authorize_handler (connection, request);
  }
}


request_handler_ptr create_post_handler (connection_ptr connection, class request * request)
{
  // Making sure Content-Type of request is something we know how to handle.
  if (request->envelope().header ("Content-Type") != "application/x-www-form-urlencoded")
    throw request_exception ("Unsupported Content-Type in POST request.");

  // Making sure there is any content in post request.
  if (request->envelope().header ("Content-Length") == "")
    throw request_exception ("A POST request must have content.");

  if (request->envelope().uri() == "/.users") {

    // Posting to authentication file.
    return create_post_users_handler (connection, request);
  } else if (request->envelope().uri().filename() == ".auth") {

    // Posting to authorization file.
    return create_post_authorization_handler (connection, request);
  } else {

    // URI does not support POST method.
    return request_handler_ptr (new error_handler (request, 403));
  }
}


request_handler_ptr create_verb_handler (connection_ptr connection, class request * request)
{
  if (request->envelope().method() == "TRACE") {

    // Returning a TRACE handler.
    return create_trace_handler (connection, request);
  } else if (request->envelope().method() == "HEAD") {

    // Returning a HEAD handler.
    return create_head_handler (connection, request);
  } else if (request->envelope().method() == "OPTIONS") {

    // Returning a OPTIONS handler.
    return create_options_handler (connection, request);
  } else if (request->envelope().method() == "GET") {

    // Returning a GET file/folder handler.
    return create_get_handler (connection, request);
  } else if (request->envelope().method() == "PUT") {

    // Returning a PUT file/folder handler.
    return create_put_handler (connection, request);
  } else if (request->envelope().method() == "DELETE") {

    // Returning a DELETE file/folder handler.
    return create_delete_handler (connection, request);
  } else if (request->envelope().method() == "POST") {

    // Returning the correct POST data handler.
    return create_post_handler (connection, request);
  } else {

    // Unsupported method.
    return request_handler_ptr (new error_handler (request, 405));
  }
}


request_handler_ptr create_request_handler (connection_ptr connection, class request * request, int status_code)
{
  // Checking if we can accept User-Agent according whitelist and blacklist definitions.
  if (!in_user_agent_whitelist (connection, request) || in_user_agent_blacklist (connection, request)) {

    // User-Agent not accepted!
    return request_handler_ptr (new error_handler (request, 403));
  }

  // Checking request type, and other parameters, deciding which type of request handler we should create.
  if (status_code >= 400) {

    // Some sort of error.
    return request_handler_ptr (new error_handler (request, status_code));
  }

  // Checking if we should upgrade an insecure request to a secure request.
  if (should_upgrade_insecure_requests (connection, request)) {

    // Both configuration, and client, prefers secure requests, and current connection is not secure, hence we upgrade.
    return upgrade_insecure_request (connection, request);
  }

  // Checking if client wants to force an authorized request.
  if (request->envelope().has_parameter ("authorize") && !request->envelope().ticket().authenticated()) {

    // Returning an Unauthorized response, to force client to authenticate.
    return create_authorize_handler (connection, request);
  }

  // Letting our verb parser take care of this.
  return create_verb_handler (connection, request);
}


} // namespace http_server
} // namespace rosetta
