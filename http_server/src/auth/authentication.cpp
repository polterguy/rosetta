
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


authentication::authentication (io_service & service, const path & auth_file)
  : _auth_file (auth_file),
    _strand (service),
    _service (service)
{
  if (auth_file.size() != 0)
    initialize();
}


void authentication::authenticate (const string & username,
                                   const string & password,
                                   const string & server_salt,
                                   authenticated_success_handler on_success)
{
  // Creating a sha1 out of password + server_salt, and base64 encoding the results, before doing comparison against password in auth file.
  auto to_hash = password + server_salt;
  auto sha1 = sha1::compute ( {to_hash.begin(), to_hash.end ()} );
  string base64_password;
  base64::encode ( {sha1.begin(), sha1.end()}, base64_password);

  // Making sure we synchronize access to users.
  _strand.dispatch ([this, username, base64_password, on_success] () {

    // Finding user with username and password combination
    auto user_iter = _users.find (username);
    if (user_iter == _users.end() || user_iter->second.password != base64_password)
      on_success (ticket ());
    else
      on_success (ticket {user_iter->second.username, user_iter->second.role});
  });
}


void authentication::change_password (const string & username,
                                      const string & password,
                                      const string & server_salt,
                                      success_handler on_success)
{
  // Creating a sha1 out of password + server_salt, and base64 encoding the results, before changing password inside our strand.
  auto to_hash = password + server_salt;
  auto sha1 = sha1::compute ( {to_hash.begin(), to_hash.end ()} );
  string base64_password;
  base64::encode ( {sha1.begin(), sha1.end()}, base64_password);

  // Making sure we synchronize access to users.
  _strand.dispatch ([this, username, base64_password, on_success] () {

    // Finding user with username and password combination
    auto user_iter = _users.find (username);
    bool found;
    if (user_iter != _users.end()) {

      // Username match
      user_iter->second.password = base64_password;
      found = true;

      // Serializing updated ".auth" file.
      save ();
    } else {

      // No such user
      found = false;
    }

    // Making sure we post on_success as "work", to release strand ASAP, in case on_success() is expensive to execute.
    _service.post ([on_success, found] () {
      
      // Invoking callback, with a boolean value, indicating whether or not a user's password actually was updated or not.
      on_success (found);
    });
  });
}


void authentication::change_role (const string & username, const string & role, success_handler on_success)
{
  // Making sure we synchronize access to users.
  _strand.dispatch ([this, username, role, on_success] () {

    // Finding user with username and password combination
    auto user_iter = _users.find (username);
    bool found;
    if (user_iter != _users.end()) {

      // Username match
      user_iter->second.role = role;
      found = true;

      // Serializing updated ".auth" file.
      save ();
    } else {

      // No such user
      found = false;
    }

    // Making sure we post on_success as "work", to release strand ASAP, in case on_success() is expensive to execute.
    _service.post ([on_success, found] () {
      
      // Invoking callback, with a boolean value, indicating whether or not a user's password actually was updated or not.
      on_success (found);
    });
  });
}


void authentication::create_user (const string & username,
                                  const string & password,
                                  const string & role,
                                  const string & server_salt,
                                  success_handler on_success)
{
  // Creating a sha1 out of password + server_salt, and base64 encoding the results, before creating our new user.
  auto to_hash = password + server_salt;
  auto sha1 = sha1::compute ( {to_hash.begin(), to_hash.end ()} );
  string base64_password;
  base64::encode ( {sha1.begin(), sha1.end()}, base64_password);

  // Making sure we synchronize access to users.
  _strand.dispatch ([this, username, base64_password, role, on_success] () {

    // Checking if user with the same name exists from before.
    auto user_iter = _users.find (username);
    bool found;
    if (user_iter == _users.end()) {

      // Username is not taken.
      found = false;

      // Adding new user object to users.
      _users[username] = {username, base64_password, role};

      // Serializing updated ".auth" file.
      save ();
    } else {

      // User already exists.
      found = true;
    }

    // Making sure we post on_success as "work", to release strand ASAP, in case on_success() is expensive to execute.
    _service.post ([on_success, found] () {
      
      // Invoking callback, with a boolean value, indicating whether or not a user actually was created or not.
      on_success (!found);
    });
  });
}


void authentication::delete_user (const string & username, success_handler on_success)
{
  // Making sure we synchronize access to users.
  _strand.dispatch ([this, username, on_success] () {

    // Checking if user exists.
    auto user_iter = _users.find (username);
    bool found;
    if (user_iter != _users.end()) {

      // Username exists.
      found = true;

      // Deleting user from map.
      _users.erase (username);

      // Serializing updated ".auth" file.
      save ();
    } else {

      // User did not exist.
      found = false;
    }

    // Making sure we post on_success as "work", to release strand ASAP, in case on_success() is expensive to execute.
    _service.post ([on_success, found] () {
      
      // Invoking callback, with a boolean value, indicating whether or not a user actually was created or not.
      on_success (!found);
    });
  });
}


void authentication::initialize ()
{
  // Opening authentication file given.
  ifstream af (_auth_file);
  if (!af.good())
    throw server_exception ("Couldn't open authentication file for server.");

  // Reading all users from authentication file.
  while (!af.eof()) {

    // Fetching next user from authentication file.
    string line;
    getline (af, line);
    trim (line);
    if (line.size() == 0)
      continue;
    vector<string> entities;
    split (entities, line, boost::is_any_of (":"));
    if (entities.size() != 3)
      throw server_exception ("Authentication file is corrupted.");
    if (_users.find (entities[0]) != _users.end())
      throw server_exception ("Authentication file is corrupted, same user is listed multiple times.");

    // Inserting username as tuple with password and role user belongs to.
    _users [entities[0]] = user {entities[0], entities[1], entities[2]};
  }
}


void authentication::save ()
{
  ofstream fs (_auth_file, std::ios::trunc | std::ios::out);
  if (!fs.good ())
    throw server_exception ("Couldn't open authentication file for writing.");
  for (auto & idx : _users) {
    fs << idx.first << ":" << idx.second.password << ":" << idx.second.role << std::endl;
  }
}


} // namespace http_server
} // namespace rosetta
