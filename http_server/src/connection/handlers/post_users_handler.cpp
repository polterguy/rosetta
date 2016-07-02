
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

#include <memory>
#include <vector>
#include <istream>
#include <boost/algorithm/string.hpp>
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
using namespace boost::algorithm;
using namespace boost::asio::detail;
using namespace rosetta::common;


post_users_handler::post_users_handler (class connection * connection, class request * request)
  : request_handler_base (connection, request)
{ }


void post_users_handler::handle (exceptional_executor x, functor on_success)
{
  // Setting deadline timer for content read.
  const int POST_CONTENT_READ_TIMEOUT = connection()->server()->configuration().get<int> ("request-post-content-read-timeout", 30);
  connection()->set_deadline_timer (POST_CONTENT_READ_TIMEOUT);

  // Reading content of request.
  auto content_length = get_content_length();
  connection()->socket().async_read (connection()->buffer(),
                                     transfer_exactly_t (content_length),
                                     [this, x, on_success] (auto error, auto bytes_read) {

    // Checking that no socket errors occurred.
    if (error)
      throw request_exception ("Socket error while reading request content.");

    // Reading content into stream;
    istream stream (&connection()->buffer());
    shared_ptr<unsigned char> buffer (new unsigned char [bytes_read], std::default_delete<unsigned char []>());
    stream.read (reinterpret_cast<char*> (buffer.get()), bytes_read);

    // Creating a string out of content, before splitting it into each name/value pair.
    string str_content (buffer.get(), buffer.get () + bytes_read);
    vector<string> parameters;
    split (parameters, str_content, boost::is_any_of ("&"));
    for (auto & idx : parameters) {
      vector<string> name_value;
      split (name_value, idx, boost::is_any_of ("="));

      // Making sure there is both a name and a value in current parameter.
      if (name_value.size() != 2)
        throw request_exception ("Bad data found in POST request.");

      // Retrieving name and value.
      string name  = uri_encode::decode (name_value [0]);
      string value = uri_encode::decode (name_value [1]);

      // Sanitizing both name and value.
      for (auto idx : name) {
        if (idx < 33 || idx > 126)
          throw request_exception ("Bad characters found in POST request content.");
      }
      for (auto idx : value) {
        if (idx < 33 || idx > 126)
          throw request_exception ("Bad characters found in POST request content.");
      }
      _parameters.push_back ( {name, value} );
    }

    // Evaluates request, now that we have the data supplied by client.
    evaluate (x, on_success);
  });
}


void post_users_handler::evaluate (exceptional_executor x, functor on_success)
{
  // Finding out which action this request wants to perform.
  auto action_iter = std::find_if (_parameters.begin(), _parameters.end(), [] (auto & idx) {
    return std::get<0> (idx) == "action";
  });

  // Retrieving the action client wants to perform.
  if (action_iter == _parameters.end ())
    throw request_exception ("Unrecognized HTTP POST request, missing 'action' parameter."); // Not recognized, hence a "bug".
  string action = std::get<1> (*action_iter);

  // Checking if client is authenticated as root, which has extended privileges.
  if (request()->envelope().ticket().role == "root") {

    // Some root account is trying to perform an action.
    root_action (action, x, on_success);
  } else if (request()->envelope().ticket().authenticated()) {

    // Some authenticated, but non-root account, is trying to perform an action
    non_root_action (action, x, on_success);
  } else {

    // Only authenticated clients are allowed to do anything here!
    throw request_exception ("Non-authenticated client tried to POST.");
  }
}


void post_users_handler::root_action (const string & action, exceptional_executor x, functor on_success)
{
  // Root is allowed to;
  // * Change password of his own account
  // * Change passwords of other accounts
  // * Change role of other accounts
  // * Create new accounts
  // * Delete accounts
  if (action == "change-password") {
    
    // Some root user is trying to change password of some user.
    root_change_password (x, on_success);
  } else if (action == "change-role") {

    // Some root user is trying to change the role of (hopefully) some other user.
    root_change_role (x, on_success);
  } else if (action == "create-user") {

    // Some root user is trying to create a new user.
    root_create_user (x, on_success);
  } else if (action == "delete-user") {

    // Some root account is trying to delete a user.
    root_delete_user (x, on_success);
  }
}


void post_users_handler::root_change_password (exceptional_executor x, functor on_success)
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
  change_password (username, new_password, x, on_success);
}


void post_users_handler::root_change_role (exceptional_executor x, functor on_success)
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
  connection()->server()->authentication().change_role (username, role, [this, x, on_success] (bool success) {

    // Returning success back to client, if user existed, and operation was successful.
    if (success)
      write_success_envelope (x, on_success);
    else
      throw request_exception ("No such user.");
  });
}


void post_users_handler::root_create_user (exceptional_executor x, functor on_success)
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
  connection()->server()->authentication().create_user (username,
                                                        password,
                                                        role,
                                                        connection()->server()->configuration().get<string> ("server-salt"),
                                                        [this, x, on_success] (bool success) {

    // Returning success back to client, if user was successfully created.
    if (success)
      write_success_envelope (x, on_success);
    else
      throw request_exception ("There is an existing user with the same username.");
  });
}


void post_users_handler::root_delete_user (exceptional_executor x, functor on_success)
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
  connection()->server()->authentication().delete_user (username, [this, x, on_success] (bool success) {

    // Returning success back to client, if user was successfully created.
    if (success)
      write_success_envelope (x, on_success);
    else
      throw request_exception ("There is an existing user with the same username.");
  });
}


void post_users_handler::non_root_action (const string & action, exceptional_executor x, functor on_success)
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
  change_password (request()->envelope().ticket().username, new_password, x, on_success);
}


void post_users_handler::change_password (const string & username, const string & new_password, exceptional_executor x, functor on_success)
{
  connection()->server()->authentication().change_password (request()->envelope().ticket().username,
                                                            new_password,
                                                            connection()->server()->configuration().get <string> ("server-salt"),
                                                            [this, x, on_success] (bool success) {

    // Returning success back to client, if user existed.
    if (success)
      write_success_envelope (x, on_success);
    else
      throw request_exception ("No such user.");
  });
}


void post_users_handler::write_success_envelope (exceptional_executor x, functor on_success)
{
  // Writing status code success back to client.
  write_status (200, x, [this, on_success] (auto x) {

    // Writing standard headers back to client.
    write_standard_headers (x, [this, on_success] (auto x) {

      // Ensuring envelope is closed.
      ensure_envelope_finished (x, on_success);
    });
  });
}


size_t post_users_handler::get_content_length ()
{
  // Max allowed length of content.
  const size_t MAX_POST_REQUEST_CONTENT_LENGTH = connection()->server()->configuration().get<size_t> ("max-request-content-length", 4096);

  // Checking if there is any content first.
  string content_length_str = request()->envelope().header ("Content-Length");

  // Checking if there is any Content-Length
  if (content_length_str.size() == 0) {

    // No content.
    throw request_exception ("No Content-Length header in PUT request.");
  } else {

    // Checking that Content-Length does not exceed max request content length.
    auto content_length = boost::lexical_cast<size_t> (content_length_str);
    if (content_length > MAX_POST_REQUEST_CONTENT_LENGTH)
      throw request_exception ("Too much content in request for server to handle.");

    // Checking that Content-Length is not 0, or negative.
    if (content_length <= 0)
      throw request_exception ("No content provided to PUT handler.");

    // Returning Content-Length to caller.
    return content_length;
  }
}


} // namespace http_server
} // namespace rosetta
