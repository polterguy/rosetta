
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

#ifndef ROSETTA_SERVER_AUTHORIZATION_HPP
#define ROSETTA_SERVER_AUTHORIZATION_HPP

#include <set>
#include <map>
#include <boost/filesystem.hpp>
#include <boost/noncopyable.hpp>
#include "http_server/include/auth/authentication.hpp"

using std::set;
using std::map;
using std::string;
using namespace boost::filesystem;

typedef map<string, set<string>> verb_roles;
typedef map<string, verb_roles> access_right;

namespace rosetta {
namespace http_server {

class server;

typedef std::function<void(bool)> success_handler;


/// Responsible for authorizing a client.
class authorization final : boost::noncopyable
{
public:

  /// Authorize a client's ticket.
  bool authorize (const authentication::ticket & ticket, class path path, const string & verb) const;

  /// Updating a specific folder's authorization access rights.
  void update (class path path, const string & verb, const string & new_value);

private:

  /// Making sure only server class can create instances.
  friend class server;

  /// Creates a new authorization object.
  authorization (const path & www_root);

  /// Implementation of authorizing a client's ticket.
  bool authorize_implementation (const authentication::ticket & ticket, class path path, const string & verb) const;

  /// Initializes authorization object.
  void initialize (const path folder);


  /// Root path for server's "www-root" folder.
  path _www_root;

  /// Folders with explicit access rights.
  access_right _access;
};


} // namespace http_server
} // namespace rosetta

#endif // ROSETTA_SERVER_AUTHORIZATION_HPP
