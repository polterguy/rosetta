
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
#include "http_server/include/auth/authentication.hpp"
#include "http_server/include/exceptions/server_exception.hpp"
#include "http_server/include/exceptions/security_exception.hpp"

using std::vector;
using std::getline;
using boost::split;

namespace rosetta {
namespace http_server {


authentication::authentication (path auth_file)
  : _auth_file (auth_file)
{
  if (auth_file.size() != 0)
    initialize();
}


const authentication::ticket authentication::authenticate (string username, string password) const
{
  auto user_iter = _users.find (username);
  if (user_iter == _users.end() || user_iter->second.password != password)
    throw security_exception ("Username/password mismatch.");

  return ticket {user_iter->second.username, user_iter->second.role};
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


} // namespace http_server
} // namespace rosetta
