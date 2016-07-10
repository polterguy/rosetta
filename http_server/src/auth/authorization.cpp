
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
  // Recursively traverse all folders in www-root of web server, to initialize authorization.
  initialize (_www_root);
}


void authorization::initialize (const path folder)
{
  // Checking if there exists an "auth" file for currently iterated folder.
  path auth_file_path = folder;
  auth_file_path += "/.auth";
  if (exists (auth_file_path)) {

    // There exists an authorization file for currently iterated folder.
    std::ifstream auth_file (auth_file_path.string (), std::ios::in);
    if (!auth_file.good())
      throw security_exception ("Couldn't open auth file; '" + auth_file_path.string() + "'.");

    // Creating an access object for currently iterated path.
    // Notice, we retrieve currently iterated folder by reference, such that changes to it, propagates back into _access member.
    verb_roles & verbs_for_folder = _access [folder.string ()];

    // Reading authorization file.
    while (!auth_file.eof ()) {

      // Reading next verb definition from currently iterated authorization file.
      string line;
      getline (auth_file, line);
      trim (line);
      if (line.size() == 0)
        continue; // Empty line, ignoring.

      // Chopping up line into VERB:role(s)
      vector<string> verb_roles;
      split (verb_roles, line, boost::is_any_of (":"));
      if (verb_roles.size() != 2)
        security_exception ("Syntax error in authorization file; '" + auth_file_path.string() + "'.");

      // Sanity checking line in authorization file.
      if (verb_roles[0] != "GET" && verb_roles[0] != "DELETE" && verb_roles[0] != "PUT" && verb_roles[0] != "HEAD" && verb_roles[0] != "TRACE")
        throw security_exception ("Malformed authorization file; '" + auth_file_path.string () + "'.");

      // Retrieving all roles associated with verb.
      vector<string> roles;
      split (roles, verb_roles[1], boost::is_any_of ("|"));
      if (roles.size() == 1 && roles[0] == "*") {

        // All roles are allowed to exercise this verb.
        verbs_for_folder [verb_roles[0]].insert ("*");
      } else {

        // Looping through all roles explicitly mentioned.
        for (auto & idxRole : roles) {
          trim (idxRole);

          // Adding currently iterated role to currently iterated verb for currently iterated folder.
          verbs_for_folder [verb_roles[0]].insert (idxRole);
        }
      }
    }

    // Then recursively iterate all child folders.
    directory_iterator iter_folders {folder};
    while (iter_folders != directory_iterator {}) {

      // Making sure we skip invisible folders.
      if (*iter_folders->path().filename().string().begin() == '.') {
        ++iter_folders;
        continue;
      }

      // Checking if this is a folder, and if so, recursively invoke self
      if (is_directory (*iter_folders)) {
        initialize (*iter_folders);
      }

      // Incrementing iterator to find next sub-folder of currently iterated folder.
      ++iter_folders;
    }
  }
}


bool authorization::authorize (const authentication::ticket & ticket, class path path, const string & verb) const
{
  if (ticket.role == "root") {

    // Root is allowed to do everything!
    return true;
  } else if (verb == "POST" && ticket.authenticated() && path == "/.users") {

    // All authenticated users are allowed to change their passwords, using POST towards "/.users" file.
    return true;
  } else {

    // Invoking implementation of authorization logic.
    return authorize_implementation (ticket, path, verb);
  }
}


bool authorization::authorize_implementation (const authentication::ticket & ticket, class path path, const string & verb) const
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

        // Role from ticket found for verb in folder; ACCESS GRANTED!
        return true;
      } else {

        // NO ACCESS, unless everybody can exercise verb!
        return iter_verb->second.find ("*") != iter_verb->second.end();
      }
    } else {

      // No explicit rights for verb, recursively invoking self for parent folder, but only if this is not the "www-root" path.
      if (path == _www_root) {

        // Defaulting to ACCESS DENIED for everything except "GET" method!
        return verb == "GET";
      } else {

        // Recursively invoking self with parent's path.
        return authorize_implementation (ticket, path.parent_path(), verb);
      }
    }
  } else {

    // No explicit rights for folder, recursively invoking self for parent folder, but only if this is not the "www-root" path.
    if (path == _www_root) {

      // Defaulting to ACCESS DENIED for everything except "GET" verb!
      return verb == "GET";
    } else {

      // Recursively invoking self with parent's path.
      return authorize_implementation (ticket, path.parent_path(), verb);
    }
  }
}


void authorization::update (class path path, const string & verb, const string & new_value)
{
  // Sanity checking new value for verb.
  for (auto idx : new_value) {
    if (idx < 32 || idx > 126)
      throw security_exception ("Illegal value for verb.");
  }

  // Sanity checking name of verb.
  if (verb != "GET" && verb != "PUT" && verb != "DELETE" && verb != "TRACE" && verb != "HEAD")
    throw security_exception ("Illegal verb."); // Notice, POST cannot have its access rights changed.

  // Doing actual update, first erasing old value for verb.
  verb_roles & roles_for_verb = _access [path.string()];
  auto existing_iter = roles_for_verb.find (verb);
  if (existing_iter != roles_for_verb.end())
    roles_for_verb.erase (existing_iter);

  // Then adding new value for verb.
  vector<string> roles;
  split (roles, new_value, boost::is_any_of ("|"));
  roles.erase (std::find (roles.begin(), roles.end(), ""));

  // Looping through all roles supplied by caller.
  auto & roles_set = roles_for_verb [verb];
  for (auto & idxRole : roles) {
    roles_set.insert (idxRole);
  }

  // Saving updated authorization file to disc.
  path += "/.auth";
  ofstream fs (path, std::ios::trunc | std::ios::out);
  if (!fs.good())
    throw security_exception ("Couldn't open authorization file for writing.");
  for (auto & idxVerb : roles_for_verb) {
    fs << idxVerb.first << ":";
    bool first = true;
    for (auto & idxRole : idxVerb.second) {
      if (first)
        first = false;
      else
        fs << "|";
      fs << idxRole;
    }
    fs << std::endl;
  }
}


} // namespace http_server
} // namespace rosetta
