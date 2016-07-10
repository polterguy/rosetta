
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

#include <vector>
#include <fstream>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <common/include/sha1.hpp>
#include <common/include/base64.hpp>
#include "http_server/include/auth/authentication.hpp"
#include "http_server/include/exceptions/server_exception.hpp"
#include "http_server/include/exceptions/security_exception.hpp"

using std::vector;
using std::getline;
using boost::trim;
using boost::split;

namespace rosetta {
namespace http_server {


authentication::authentication ()
{
  // Opening authentication file given.
  ifstream auth_file (".users");
  if (!auth_file.good())
    throw server_exception ("Couldn't open authentication file for server.");

  // Reading all users from authentication file.
  while (!auth_file.eof()) {

    // Fetching next user from authentication file.
    string line;
    getline (auth_file, line);
    trim (line);
    if (line.size() == 0)
      continue; // Empty line

    // Splitting line into username:password:role.
    vector<string> entities;
    split (entities, line, boost::is_any_of (":"));

    // Basic sanity check.
    if (entities.size() != 3)
      throw server_exception ("Authentication file is corrupted.");
    if (_users.find (entities[0]) != _users.end())
      throw server_exception ("Authentication file is corrupted, same user is listed multiple times.");

    // Inserting username as tuple with password and role user belongs to.
    _users [entities[0]] = user {entities[0], entities[1], entities[2]};
  }
}


authentication::ticket authentication::authenticate (const string & username, const string & password, const string & server_salt) const
{
  // Creating a sha1 out of password + server_salt, and base64 encoding the results, before doing comparison against password in auth file.
  auto to_hash = password + server_salt;
  auto sha1 = sha1::compute ( {to_hash.begin(), to_hash.end ()} );
  string base64_password;
  base64::encode ( {sha1.begin(), sha1.end()}, base64_password);

  // Finding user with username and password combination
  auto user_iter = _users.find (username);
  if (user_iter == _users.end() || user_iter->second.password != base64_password) {

    // Couldn't authenticate client.
    throw security_exception ("No such user.");
  } else {

    // Authenticated!
    return ticket {user_iter->second.username, user_iter->second.role};
  }
}


void authentication::change_password (const string & username, const string & password, const string & server_salt)
{
  // Creating a sha1 out of password + server_salt, and base64 encoding the results, which becomes the password to put into auth file.
  auto to_hash = password + server_salt;
  auto sha1 = sha1::compute ( {to_hash.begin(), to_hash.end ()} );
  string base64_password;
  base64::encode ( {sha1.begin(), sha1.end()}, base64_password);

  // Finding user with username and password combination
  auto user_iter = _users.find (username);
  if (user_iter != _users.end()) {

    // Username match, updating password before saving auth file.
    user_iter->second.password = base64_password;
    save ();
  } else {

    // No such user.
    throw security_exception ("No such user.");
  }
}


void authentication::change_role (const string & username, const string & role)
{
  // Finding user with username and password combination
  auto user_iter = _users.find (username);
  if (user_iter != _users.end()) {

    // Username match, updating role before saving auth file.
    user_iter->second.role = role;
    save ();
  } else {

    // No such user.
    throw security_exception ("No such user.");
  }
}


void authentication::create_user (const string & username, const string & password, const string & role, const string & server_salt)
{
  // Creating a sha1 out of password + server_salt, and base64 encoding the results, before creating our new user.
  auto to_hash = password + server_salt;
  auto sha1 = sha1::compute ( {to_hash.begin(), to_hash.end ()} );
  string base64_password;
  base64::encode ( {sha1.begin(), sha1.end()}, base64_password);

  // Checking if user with the same name exists from before.
  auto user_iter = _users.find (username);
  if (user_iter == _users.end()) {

    // Username is not taken, creating user before saving auth file.
    _users [username] = {username, base64_password, role};
    save ();
  } else {

    // User already exists.
    throw security_exception ("User already exists.");
  }
}


void authentication::delete_user (const string & username)
{
  // Checking if user exists.
  auto user_iter = _users.find (username);
  if (user_iter != _users.end()) {

    // Deleting user, before saving updated auth file.
    _users.erase (username);
    save ();
  } else {

    // User did not exist.
    throw security_exception ("No such user.");
  }
}


void authentication::save ()
{
  // Saving file.
  ofstream fs (".users", std::ios::trunc | std::ios::out);
  if (!fs.good ())
    throw server_exception ("Couldn't open authentication file for writing.");
  for (auto & idx : _users) {
    fs << idx.first << ":" << idx.second.password << ":" << idx.second.role << std::endl;
  }
}


} // namespace http_server
} // namespace rosetta
