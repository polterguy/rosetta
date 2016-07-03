
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
#include <string>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include "http_server/include/auth/authorization.hpp"
#include "http_server/include/exceptions/security_exception.hpp"

using std::string;
using std::vector;
using std::getline;
using boost::trim;
using boost::split;

namespace rosetta {
namespace http_server {


authorization::authorization (const path & www_root)
  : _www_root (www_root)
{
  // Recursively traverse all folders of web server, to initialize authorization.
  initialize (_www_root);
}


void authorization::initialize (const path current)
{
  // Checking if there exists an "auth" file for currently iterated folder.
  path idx = current;
  idx += "/.auth";
  if (exists (idx)) {
    std::ifstream auth_file (idx.string (), std::ios::in);
    if (!auth_file.good())
      throw security_exception ("Couldn't open auth file; '" + idx.string() + "'.");

    // Creating an access object for currently iterated path.
    verb_roles & folder_rights = _access [current.string ()];

    // Reading authorization file.
    while (!auth_file.eof ()) {
      string line;
      getline (auth_file, line);
      trim (line);
      if (line.size() == 0)
        continue;

      // Chopping up line into entities.
      vector<string> entities;
      split (entities, line, boost::is_any_of (":"));
      if (entities.size() != 2)
        security_exception ("Syntax error in authorization file; '" + idx.string() + "'.");

      // Retrieving all roles associated with verb.
      vector<string> roles;
      split (roles, entities[1], boost::is_any_of ("|"));
      if (roles.size() == 1 && roles[0] == "*") {

        // All roles are allowed to exercise this verb.
        folder_rights [entities[0]].insert ("*");
      } else {

        // Looping through all roles explicitly mentioned.
        for (auto & idxRole : roles) {
          trim (idxRole);
          if (idxRole == "*" || idxRole.size() == 0)
            throw security_exception ("Malformed authorization file; '" + idx.string () + "'.");
          folder_rights [entities[0]].insert (idxRole);
        }
      }
    }

    // Then recursively iterate all child folders.
    directory_iterator iter_folders {current};
    while (iter_folders != directory_iterator{}) {

      // Checking if this is a folder, and if so, recursively invoke self
      if (is_directory (*iter_folders)) {
        initialize (*iter_folders);
      }
      ++iter_folders;
    }
  }
}


void authorization::authorize (const authentication::ticket & ticket, class path path, const string & verb, success_handler on_success) const
{
  if (ticket.role == "root") {

    // Root is allowed to do everything!
    on_success (true);
  } else {

    // Invoking implementation of authorization logic.
    authorize_implementation (ticket, path, verb, on_success);
  }
}


void authorization::authorize_implementation (const authentication::ticket & ticket,
                                              class path path,
                                              const string & verb,
                                              success_handler on_success) const
{
  // Checking if directory exists in "explicit user access folders".
  auto iter_folder = _access.find (path.string ());
  if (iter_folder != _access.end ()) {

    // Folder has explicit rights, now checking if verb is mentioned.
    auto iter_verb = iter_folder->second.find (verb);
    if (iter_verb != iter_folder->second.end()) {

      // Verb is explicitly mentioned, now checking if ticket's role is mentioned in verb.
      auto iter_role = iter_verb->second.find (ticket.role);
      if (iter_role != iter_verb->second.end()) {

        // Role found for verb in folder; ACCESS GRANTED!
        on_success (true);
      } else {

        // NO ACCESS, unless everybody can exercise verb!
        on_success (iter_verb->second.find ("*") != iter_verb->second.end());
      }
    } else {

      // No explicit rights for verb, recursively invoking self for parent folder, but only if this is not the "www-root" path.
      if (path == _www_root) {

        // Defaulting to ACCESS DENIED for everything except "GET" method!
        on_success (verb == "GET");
      } else {

        // Recursively invoking self with parent's path.
        authorize_implementation (ticket, path.parent_path(), verb, on_success);
      }
    }
  } else {

    // No explicit rights for folder, recursively invoking self for parent folder, but only if this is not the "www-root" path.
    if (path == _www_root) {

      // Defaulting to ACCESS DENIED for everything except "GET" method!
      on_success (verb == "GET");
    } else {

      // Recursively invoking self with parent's path.
      authorize_implementation (ticket, path.parent_path(), verb, on_success);
    }
  }
}


void authorization::update (class path path, const string & verb, const string & new_value, success_handler on_success)
{
}


} // namespace http_server
} // namespace rosetta
