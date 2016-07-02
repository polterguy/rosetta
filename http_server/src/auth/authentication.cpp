
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


authentication::authentication (const path & auth_file)
  : _auth_file (auth_file)
{
  if (auth_file.size() != 0)
    initialize();
}


authentication::ticket authentication::authenticate (const string & username, const string & password, const string & server_salt) const
{
  // Creating a sha1 out of password + server_salt, and base64 encoding the results, before doing comparison against password in auth file.
  auto to_hash = password + server_salt;
  auto sha1 = sha1::compute ( {to_hash.begin(), to_hash.end ()} );
  string base64_password;
  base64::encode ( {sha1.begin(), sha1.end()}, base64_password);

  auto user_iter = _users.find (username);
  if (user_iter == _users.end() || user_iter->second.password != base64_password)
    return ticket ();

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


} // namespace http_server
} // namespace rosetta
