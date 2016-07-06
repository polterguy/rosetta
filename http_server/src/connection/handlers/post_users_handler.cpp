
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

#include "http_server/include/server.hpp"
#include "http_server/include/helpers/uri_encode.hpp"
#include "http_server/include/connection/request.hpp"
#include "http_server/include/connection/connection.hpp"
#include "http_server/include/exceptions/request_exception.hpp"
#include "http_server/include/connection/handlers/post_users_handler.hpp"

namespace rosetta {
namespace http_server {

using std::string;
using std::vector;
using std::istream;
using std::shared_ptr;
using std::default_delete;
using namespace rosetta::common;


post_users_handler::post_users_handler (class request * request)
  : post_handler_base (request)
{ }


void post_users_handler::handle (connection_ptr connection, std::function<void()> on_success)
{
  // Letting base class do the heavy lifting.
  post_handler_base::handle (connection, [this, connection, on_success] () {

    // Evaluates request, now that we have the data supplied by client.
    try {

      // Unless evaluate() throws an exception, we can safely return success back to client.
      evaluate (connection);
      write_success_envelope (connection, on_success);
    } catch (std::exception & error) {

      // Something went wrong!
      request()->write_error_response (connection, 500);
    }
  });
}


void post_users_handler::evaluate (connection_ptr connection)
{
  // Finding out which action this request wants to perform.
  auto action_iter = std::find_if (_parameters.begin(), _parameters.end(), [] (auto & idx) {
    return std::get<0> (idx) == "action";
  });

  // Retrieving the action client wants to perform.
  if (action_iter == _parameters.end ()) {

    // Oops, no "action" POST parameter given.
    throw request_exception ("Missing 'action' parameter of POST request.");
  } else {

    // Retrieving "action" client tries to execute.
    string action = std::get<1> (*action_iter);

    // Checking if client is authenticated as root, which has extended privileges.
    if (request()->envelope().ticket().role == "root") {

      // Some root account is trying to perform an action.
      root_action (connection, action);
    } else if (request()->envelope().ticket().authenticated()) {

      // Some authenticated, but non-root account, is trying to perform an action
      non_root_action (connection, action);
    } else {

      // Only authenticated clients are allowed to do anything here!
      throw request_exception ("Client is not authorized to perform this action.");
    }
  }
}


void post_users_handler::root_action (connection_ptr connection, const string & action)
{
  // Root is allowed to;
  // * Change password of his own account
  // * Change passwords of other accounts
  // * Change role of other accounts
  // * Create new accounts
  // * Delete accounts
  if (action == "change-password") {
    
    // Some root user is trying to change password of some user.
    root_change_password (connection);
  } else if (action == "change-role") {

    // Some root user is trying to change the role of (hopefully) some other user.
    root_change_role (connection);
  } else if (action == "create-user") {

    // Some root user is trying to create a new user.
    root_create_user (connection);
  } else if (action == "delete-user") {

    // Some root account is trying to delete a user.
    root_delete_user (connection);
  }
}


void post_users_handler::root_change_password (connection_ptr connection)
{
  // Root account tries to change password, now we need to figure out if it's his password, or another user's password.
  // We default to authenticated user (root account performing action)
  string username = request()->envelope().ticket().username;

  // Checking if an explicit username was given.
  auto username_iter = std::find_if (_parameters.begin(), _parameters.end(), [] (auto & idx) {
    return std::get<0> (idx) == "username";
  });
  if (username_iter != _parameters.end())
    username = std::get<1> (*username_iter);

  // Then figuring out what the new password is.
  auto password_iter = std::find_if (_parameters.begin(), _parameters.end(), [] (auto & idx) {
    return std::get<0> (idx) == "password";
  });
  if (password_iter == _parameters.end())
    throw request_exception ("Missing 'password' parameter of POST request.");

  // Retrieving new password, and updating currently authenticated user's password.
  string new_password = std::get<1> (*password_iter);

  // Now we have all the data necessary to change password of some user account.
  change_password (connection, username, new_password);
}


void post_users_handler::root_change_role (connection_ptr connection)
{
  // Root account tries to change the role of another user.
  // First we retrieve the username of the account root is trying to change the role of.
  auto username_iter = std::find_if (_parameters.begin(), _parameters.end(), [] (auto & idx) {
    return std::get<0> (idx) == "username";
  });
  if (username_iter == _parameters.end())
    throw request_exception ("No username parameter supplied to 'change-role' action.");
  string username = std::get<1> (*username_iter);

  // Then we verify that the root user is not trying to change his own password, which is an illegal action.
  if (username == request()->envelope().ticket().username)
    throw request_exception ("Changing your own role is illegal for a root account.");

  // Then we retrieve the new role of the user.
  auto role_iter = std::find_if (_parameters.begin(), _parameters.end(), [] (auto & idx) {
    return std::get<0> (idx) == "role";
  });
  if (role_iter == _parameters.end())
    throw request_exception ("No role parameter supplied to 'change-role' action.");
  string role = std::get<1> (*role_iter);

  // Now we have all the necessary data to change the role of some account in system.
  connection->server()->authentication().change_role (username, role);
}


void post_users_handler::root_create_user (connection_ptr connection)
{
  // Root account tries to create a new user.
  // First we retrieve the username of the account root is trying to create.
  auto username_iter = std::find_if (_parameters.begin(), _parameters.end(), [] (auto & idx) {
    return std::get<0> (idx) == "username";
  });
  if (username_iter == _parameters.end())
    throw request_exception ("No username parameter supplied to 'create-user' action.");
  string username = std::get<1> (*username_iter);

  // Then we retrieve the role of the new user.
  auto role_iter = std::find_if (_parameters.begin(), _parameters.end(), [] (auto & idx) {
    return std::get<0> (idx) == "role";
  });
  if (role_iter == _parameters.end())
    throw request_exception ("No role parameter supplied to 'create-user' action.");
  string role = std::get<1> (*role_iter);

  // Then we retrieve the password of the new user.
  auto password_iter = std::find_if (_parameters.begin(), _parameters.end(), [] (auto & idx) {
    return std::get<0> (idx) == "password";
  });
  if (password_iter == _parameters.end())
    throw request_exception ("No password parameter supplied to 'create-user' action.");
  string password = std::get<1> (*password_iter);

  // Now we have all the necessary data to create a new user in system.
  connection->server()->authentication().create_user (username,
                                                      password,
                                                      role,
                                                      connection->server()->configuration().get<string> ("server-salt"));
}


void post_users_handler::root_delete_user (connection_ptr connection)
{
  // Root account tries to delete an existing (hopefully) user.
  // First we retrieve the username of the account root is trying to delete.
  auto username_iter = std::find_if (_parameters.begin(), _parameters.end(), [] (auto & idx) {
    return std::get<0> (idx) == "username";
  });
  if (username_iter == _parameters.end())
    throw request_exception ("No username parameter supplied to 'create-user' action.");
  string username = std::get<1> (*username_iter);

  // Now we have all the necessary data to create a new user in system.
  connection->server()->authentication().delete_user (username);
}


void post_users_handler::non_root_action (connection_ptr connection, const string & action)
{
  // Non-root accounts are only able to change passwords of their own account.
  // A "change my password" action, requires exactly two parameters.
  if (action != "change-password" || _parameters.size() != 2)
    throw request_exception ("Illegal 'action' of POST request.");

  // Then figuring out what the new password is.
  auto password_iter = std::find_if (_parameters.begin(), _parameters.end(), [] (auto & idx) {
    return std::get<0> (idx) == "password";
  });
  if (password_iter == _parameters.end())
    throw request_exception ("Missing 'password' parameter of POST request.");

  // Retrieving new password, and updating currently authenticated user's password.
  string new_password = std::get<1> (*password_iter);

  // Now we have all the data necessary to change password of currently authenticated client's account.
  change_password (connection, request()->envelope().ticket().username, new_password);
}


void post_users_handler::change_password (connection_ptr connection, const string & username, const string & new_password)
{
  connection->server()->authentication().change_password (request()->envelope().ticket().username,
                                                          new_password,
                                                          connection->server()->configuration().get <string> ("server-salt"));
}


} // namespace http_server
} // namespace rosetta
