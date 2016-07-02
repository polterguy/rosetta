
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

#ifndef ROSETTA_SERVER_AUTHENTICATION_HPP
#define ROSETTA_SERVER_AUTHENTICATION_HPP

#include <map>
#include <tuple>
#include <boost/filesystem.hpp>

using std::string;
using namespace boost::filesystem;

namespace rosetta {
namespace http_server {


/// Responsible for authenticate a client.
class authentication final : boost::noncopyable
{
public:

  /// Ticket class, wraps an authenticated user.
  struct ticket final
  {
    inline bool authenticated() const { return username.size() > 0; }
    string username;
    string role;
  };

  /// Creates an authentication instance.
  authentication (const path & auth_file);

  /// Authenticates a user, and returns a ticket.
  ticket authenticate (const string & username, const string & password, const string & server_salt) const;

private:

  /// Wraps a single user in system
  struct user final
  {
    string username;
    string password;
    string role;
  };

  /// Initializes the authentication object.
  void initialize ();


  /// Path to authentication file.
  path _auth_file;

  /// Users, with their usernames and roles.
  std::map<string, user> _users;
};


} // namespace http_server
} // namespace rosetta

#endif // ROSETTA_SERVER_AUTHENTICATION_HPP
